<?php
/**
 * @file
 * Contains merge process for merging fields.
 */

class EntityMergeProcessMergeFields extends EntityMergeProcessBase {

  /**
   * Overrides EntityMergeProcessBase::run()
   */
  public function run(&$context = array()) {
    list(,,$bundle) = entity_extract_ids($this->entityType, $this->targetEntity);

    $instances = field_info_instances($this->entityType, $bundle);

    foreach ($instances as $field_name => $instance) {
      $field = field_info_field($field_name);

      // Put all the field items into one items array. We make sure we don't have
      // too many items later.
      $entity1_items = is_array($this->entity1->{$field_name}) ? $this->entity1->{$field_name} : array();
      $entity2_items = is_array($this->entity2->{$field_name}) ? $this->entity2->{$field_name} : array();
      $new_items = array_merge_recursive($entity1_items, $entity2_items);

      $empty_hook = $field['module'] . '_field_is_empty';
      $empty_hook_exists = function_exists($empty_hook);

      // Set up the context for alter hooks.
      $context = array(
        'field' => $field,
        'instance' => $instance,
        'entity1' => $this->entity1,
        'entity2' => $this->entity2,
        'entity_type' => $this->entityType,
      );

      // Discard items that are empty.
      $langcodes = !empty($new_items) ? array_keys($new_items) : array(LANGUAGE_NONE);
      foreach ($langcodes as $langcode) {
        if (!isset($new_items[$langcode])) {
          $new_items[$langcode] = array();
        }

        $items = &$new_items[$langcode];
        foreach ($items as $delta => $item) {
          if (empty($item)) {
            unset($items[$delta]);
            continue;
          }

          if (!$empty_hook_exists) {
            continue;
          }

          if ($empty_hook($item, $field)) {
            unset($items[$delta]);
            continue;
          }
        }

        // Allow modules to alter the field items.
        $context['langcode'] = $langcode;
        drupal_alter('entity_merge_field_items', $items, $this->targetEntity, $context);

        // Re-index the items.
        $items = array_values($items);

        // If there are no items, remove this language.
        if (!count($items)) {
          unset($new_items[$langcode]);
        }
      }

      // Make sure we only have the right number of items.
      if ($field['cardinality'] != FIELD_CARDINALITY_UNLIMITED) {
        foreach ($new_items as &$items) {
          if (count($items) > $field['cardinality']) {
            $items = array_slice($items, 0, $field['cardinality']);
          }
        }
      }

      // Set the item array, or if empty set to NULL.
      $this->targetEntity->{$field_name} = !empty($new_items) ? $new_items : NULL;
    }

    parent::run($context);
  }

  /**
   * {@inheritdoc}
   */
  public function getProgressMessage(&$context = array()) {
    return t('Merging Field Data');
  }

  /**
   * {@inheritdoc}
   */
  public function previewRows($prefix = 'fields:') {
    list(,,$bundle) = entity_extract_ids($this->entityType, $this->targetEntity);
    $instances = field_info_instances($this->entityType, $bundle);

    $rows = array();
    $w1 = entity_metadata_wrapper($this->entityType, $this->entity1);
    $w2 = entity_metadata_wrapper($this->entityType, $this->entity2);

    foreach ($instances as $field_name => $instance) {
      try {
        $row = array(
          'data' => array(
            array('data' => $instance['label']),
          ),
        );

        $value1 = $value2 = NULL;
        $instance['display']['default']['label'] = 'hidden';
        if (!empty($this->entity1->{$field_name}[LANGUAGE_NONE])) {
          $value1 = field_view_field($this->entityType, $this->entity1, $field_name, $instance['display']['default']);
        }
        if (!empty($this->entity2->{$field_name}[LANGUAGE_NONE])) {
          $value2 = field_view_field($this->entityType, $this->entity2, $field_name, $instance['display']['default']);
        }

        if (empty($value1) && empty($value2)) {
          continue;
        }

        $row['data'][] = array(
          'class' => array('entity-merge-ui-primary'),
          'data' => $value1,
        );
        $row['data'][] = array(
          'class' => array('entity-merge-ui-duplicate'),
          'data' => $value2,
        );

        // Work out the action.
        $field_info = field_info_field($field_name);
        if ($field_info['cardinality'] == 1) {
          $v1 = $w1->{$field_name}->value();
          $v2 = $w2->{$field_name}->value();

          if ($v1 === $v2) {
            $action = 'match';
          }
          else if (!is_object($v1) && $v1 == $v2) {
            $action = 'match';
          }
          else if (empty($v2)) {
            $action = 'use_1';
          }
          else if (empty($v1)) {
            $action = 'use_2';
          }
          else {
            $action = 'conflict';
          }
        }
        else {
          if ($w1->{$field_name}->count() == 0) {
            $action = 'use_2';
          }
          else if ($w2->{$field_name}->count() == 0) {
            $action = 'use_1';
          }
          else {
            $action = 'combine';
          }
        }

        switch ($action) {
          case 'match':
            $row['class'][] = 'entity-merge-ui-match';
            $row['data'][2]['class'][] = 'entity-merge-ui-match';
            $row['data'][1]['class'][] = 'entity-merge-ui-match';
            $row['data'][1]['class'][] = 'entity-merge-ui-use';
            break;
          case 'use_2':
            $row['data'][2]['class'][] = 'entity-merge-ui-use';
            break;
          case 'use_1':
            $row['data'][1]['class'][] = 'entity-merge-ui-use';
            break;
          case 'conflict':
            $row['class'][] = 'entity-merge-ui-conflict';
            $row['data'][1]['class'][] = 'entity-merge-ui-conflict';
            $row['data'][2]['class'][] = 'entity-merge-ui-conflict';
            $row['data'][1]['class'][] = 'entity-merge-ui-use';
            break;
          case 'combine':
            $row['class'][] = 'entity-merge-ui-combine';
            $row['data'][1]['class'][] = 'entity-merge-ui-use';
            $row['data'][2]['class'][] = 'entity-merge-ui-use';
            break;
        }

        $rows[$prefix.$field_name] = $row;
      }
      catch (Exception $e) {
      }
    }

    return $rows;
  }
}
