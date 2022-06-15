<?php

/**
 * @file
 * Integrates Drupella FM into FileField.
 */

/**
 * Extends filefield_widget element.
 */
function _dfm_filefield_field_process($element, &$form_state, $form) {
  // Create url with initial location
  $options = array();
  if (!empty($element['#upload_location']) && $target = file_uri_target($element['#upload_location'])) {
    $options['query']['fallbackDir'] = $target;
  }
  // Button
  $base_id = preg_replace('/\-upload$/', '', $element['#id']);
  $element['dfm_button'] = array(
    '#id' => $base_id . '-dfm-button',
    '#name' => $element['#name'] . '[dfm_button]',
    '#attributes' => array(
      'class' => array('element-hidden', 'dfm-filefield-button'),
      'data-dfm-url' => url('dfm/' . $element['#scheme'], $options),
    ),
  ) + $element['upload_button'];
  // Path input
  $element['dfm_paths'] = array(
    '#type' => 'hidden',
    '#attributes' => array(
      'id' => $base_id . '-dfm-paths',
    ),
    // Reset value to prevent consistent errors
    '#value' => '',
  );
  // Library
  $path = drupal_get_path('module', 'dfm_filefield');
  $element['#attached']['js'][] = $path . '/dfm_filefield.js';
  $element['#attached']['css'][] = $path . '/dfm_filefield.css';
  // Set the pre-renderer to conditionally disable the elements.
  $element['#pre_render'][] = 'dfm_filefield_field_pre_render';
  return $element;
}

/**
 * Pre renderer that conditionally disables elements.
 */
function dfm_filefield_field_pre_render($element) {
  // Hide elements if there is already an uploaded file.
  if (!empty($element['#value']['fid'])) {
    $element['dfm_paths']['#access'] = FALSE;
    $element['dfm_button']['#access'] = FALSE;
    $element['remove_button']['#attributes']['class'][] = 'dfm-queue-remover';
  }
  return $element;
}

/**
 * Field value callback.
 */
function _dfm_filefield_field_value($element, &$item, &$form_state) {
  if (empty($item['dfm_paths'])) {
    return;
  }
  dfm_inc('page');
  $paths = $item['dfm_paths'];
  $item['dfm_paths'] = '';
  $paths = array_unique(array_filter(explode(':', $paths)));
  // Check if paths are accessible by the current user with Dfm.
  // D7 does not support multiple values returned by the field value callback.
  if ($uris = dfm_check_file_paths(array($paths[0]), $GLOBALS['user'], $element['#scheme'])) {
    // Get entity by uri
    $uri = reset($uris);
    $file = dfm_file_by_path($uri, TRUE);
    if (dfm_filefield_element_validate($element, $file)) {
      // Save file
      if (empty($file->fid)) {
        dfm_write_file_record($file);
      }
      // Add to input.
      if (!empty($file->fid)) {
        // Needs usage.
        if (!file_usage_list($file)) {
          dfm_claim_file($file);
        }
        $item = array_merge($item, (array) $file);
      }
    }
  }
}

/**
 * Validates a file with element validators.
 */
function dfm_filefield_element_validate($element, $file) {
  if ($errors = file_validate($file, $element['#upload_validators'])) {
    $message = count($errors) > 1 ? '<ul><li>' . implode('</li><li>', $errors) . '</li></ul>' : $errors[0];
    form_error($element, $message);
    return FALSE;
  }
  return TRUE;
}