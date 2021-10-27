<?php

/**
 * Build a merge preview table.
 */
function entity_merge_preview($entity_type, $entity_1, $entity_2, $conf = array()) {
  $info = entity_get_info($entity_type);
  if (empty($info['merge handler class']) || !class_exists($info['merge handler class'])) {
    return array(
      '#type' => 'markup',
      '#markup' => t('@entity_type entites do not support merging.', array('@entity_type' => $info['label'])),
    );
  }

  $class = $info['merge handler class'];
  $handler = new $class($entity_type, $entity_1, $entity_2, FALSE, FALSE, FALSE, $conf);

  return $handler->preview();
}

/**
 * Build a merge form.
 */
function entity_merge_form($form, &$form_state, $entity_type, $entity1 = NULL, $entity2 = NULL) {
  $form_state['entity_type'] = $entity_type;
  $info = entity_get_info($entity_type);
  if (empty($info) || empty($info['merge handler class']) || !class_exists($info['merge handler class'])) {
    return array(
      'markup' => array(
        '#markup' => t('Invalid entity type'),
      ),
    );
  }

  if (!$entity1 || !$entity2) {
    $args = array('@entities' => !empty($info['plural label']) ? $info['plural label'] : $info['label'].'s');
    $form['selection'] = array(
      '#type' => 'fieldset',
      '#title' => t('Select @entities to Merge', $args),
      '#description' => t('Entity the IDs of the @entities you wish to merge', $args),
      '#tree' => TRUE,
    );
    $form['selection']['#collapsible'] = 
      $form['selection']['#collapsed'] = $form_state['submitted'];

    $form['selection']['entity1'] = array(
      '#type' => 'textfield',
      '#title' => t('Primary @entity ID', array('@entity' => $info['label'])),
      '#description' => t('Data from the secondary @entity will be merged into this @entity', array('@entity' => $info['label'])),
      '#required' => TRUE,
    );
    $form['selection']['entity2'] = array(
      '#type' => 'textfield',
      '#title' => t('Secondary @entity ID', array('@entity' => $info['label'])),
      '#description' => t('Data from the this @entity will be merged into the primary @entity', array('@entity' => $info['label'])),
      '#required' => TRUE,
    );
    $form['selection']['actions'] = array('#type' => 'actions');
    $form['selection']['actions']['verify'] = array(
      '#type' => 'submit',
      '#value' => t('Verify'),
    );

    $entity1 = !empty($form_state['values']['selection']['entity1']) ? $form_state['values']['selection']['entity1'] : NULL;
    $entity2 = !empty($form_state['values']['selection']['entity2']) ? $form_state['values']['selection']['entity2'] : NULL;
  }

  // Add our current selection for our merge submission.
  $form['entity1'] = array(
    '#type' => 'value',
    '#value' => $entity1,
  );
  $form['entity2'] = array(
    '#type' => 'value',
    '#value' => $entity2,
  );

  if (!$entity1 || !$entity2) {
    return $form;
  }

  $entity1 = entity_load_single($entity_type, $entity1);
  $entity2 = entity_load_single($entity_type, $entity2);

  if ($entity1 && $entity2) {
    $form['merge'] = array(
      '#type' => 'container',
    );
    if (isset($form['selection'])) {
      $form['merge']['#states'] = array(
        'visible' => array(
          ':input[name="selection[entity1]"]' => array('value' => entity_id($entity_type, $entity1)),
          ':input[name="selection[entity2]"]' => array('value' => entity_id($entity_type, $entity2)),
        ),
      );
    }

    $form['merge']['verify'] = entity_merge_preview($entity_type, $entity1, $entity2);
    $form['merge']['actions'] = array(
      '#type' => 'actions',
      '#weight' => 100,
    );
    $form['merge']['actions']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Merge'),
      '#validate' => array(
        'entity_merge_form_merge_validate',
      ),
      '#submit' => array(
        'entity_merge_form_merge_submit',
      ),
    );
  }

  return $form;
}

/**
 * Validate the merge form.
 */
function entity_merge_form_validate($form, &$form_state) {
  $info = entity_get_info($form_state['entity_type']);
  if (!entity_load_single($form_state['entity_type'], $form_state['values']['selection']['entity1'])) {
    form_error($form['selection']['entity1'], t('Cannot find @entity with id %id.', array('%id' => $form_state['values']['selection']['entity1'], '@entity' => $info['label'])));
  }
  if (!entity_load_single($form_state['entity_type'], $form_state['values']['selection']['entity2'])) {
    form_error($form['selection']['entity2'], t('Cannot find @entity with id %id.', array('%id' => $form_state['values']['selection']['entity2'], '@entity' => $info['label'])));
  }
}

/**
 * Submit the merge selection form.
 */
function entity_merge_form_submit($form, &$form_state) {
  $form_state['rebuild'] = TRUE;
}

/**
 * Validate the run merge form.
 */
function entity_merge_form_merge_validate($form, &$form_state) {
  if (isset($form_state['values']['selection']['entity1'])) {
    if ($form_state['values']['selection']['entity1'] != $form_state['values']['entity1']) {
      form_error($form['selection']['entity1'], t('Please verify your new selection.'));
    }
  }
  if (isset($form_state['values']['selection']['entity2'])) {
    if ($form_state['values']['selection']['entity2'] != $form_state['values']['entity2']) {
      form_error($form['selection']['entity2'], t('Please verify your new selection.'));
    }
  }
}

/**
 * Submit the run merge form.
 */
function entity_merge_form_merge_submit($form, &$form_state) {
  $entity1 = entity_load_single($form_state['entity_type'], $form_state['values']['entity1']);
  $entity2 = entity_load_single($form_state['entity_type'], $form_state['values']['entity2']);

  $config = isset($form_state['values']['config']) ? $form_state['values']['config'] : array();
  $class = entity_get_info($form_state['entity_type'])['merge handler class'];
  $handler = new $class($form_state['entity_type'], $entity1, $entity2, FALSE, FALSE, FALSE, $config);
  $handler->prepare()->batchGo();
}
