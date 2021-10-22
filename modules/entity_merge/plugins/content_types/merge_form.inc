<?php

$plugin = array(
  'title' => t('Entity Merge Form'),
  'defaults' => array(),
  'content type' => 'entity_merge_merge_form_content_type_content_type',
);

function entity_merge_merge_form_content_type_content_type($subtype) {
  $types = entity_merge_merge_form_content_type_content_types();
  if (isset($types[$subtype])) {
    return $types[$subtype];
  }
}

/**
 * Get all entity merge form sub types.
 */
function entity_merge_merge_form_content_type_content_types() {
  $types = &drupal_static(__FUNCTION__, array());
  if (!empty($types)) {
    return $types;
  }

  foreach (entity_get_info() as $entity_type => $info) {
    if (empty($info['merge handler class']) || !class_exists($info['merge handler class'])) {
      continue;
    }

    $args = array('@entity_type' => $info['label']);
    $types[$entity_type] = array(
      'category' => t('Entity Merge'),
      'title' => t('Merge @entity_type Form', $args),
      'description' => t('Show a form to merge @entity_type entities', $args),
      'edit form' => 'entity_merge_merge_form_content_type_options',
      'required context' => array(
        new ctools_context_optional(t('Primary @entity_type', $args), $entity_type),
        new ctools_context_optional(t('Duplicate @entity_type', $args), $entity_type),
      ),
    );
  }

  return $types;
}

function entity_merge_merge_form_content_type_options($form, &$form_state) {
  return $form;
}

function entity_merge_merge_form_content_type_options_submit($form, &$form_state) {}

function entity_merge_merge_form_content_type_admin_title($subtype, $conf, $context) {
  $info = entity_get_info($subtype);
  return t('@entity_type Merge Form', array('@entity_type' => $info['label']));
}

function entity_merge_merge_form_content_type_render($subtype, $conf, $panel_args, $context) {
  $entity1 = !empty($context[0]->data) ? $context[0]->data : NULL;
  $entity2 = !empty($context[1]->data) ? $context[1]->data : NULL;
  $entity_info = entity_get_info($subtype);

  $block = new stdClass();
  $block->module = 'entity_merge';
  $block->delta = 'merge_form__'.$subtype;
  $block->title = t('Merge @entity_type', array('@entity_type' => !empty($entity_info['plural_label']) ? $entity_info['plural_label'] : $entity_info['label'].'s'));
  $block->content = drupal_get_form(
    'entity_merge_'.$subtype.'_form',
    entity_id($subtype, $entity1),
    entity_id($subtype, $entity2)
  );

  return $block;
}
