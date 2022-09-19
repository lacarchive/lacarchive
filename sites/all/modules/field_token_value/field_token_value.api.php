<?php

/**
 * @file
 * Contains hooks provided by the Field Token Value module.
 */

/**
 * @addtogroup hooks
 * @{
 */

/**
 * Provide a wrapper to the field formatter for the field_token_value field.
 *
 * @return array
 *   A keyed array containing information for the wrapper.
 */
function hook_field_token_value_wrapper_info() {
  return array(
    'p' => array(
      // The title used within the select list. This is required.
      'title' => t('Paragraph'),
      // A summary of the wrapper to display as the field formatter summary.
      // This is optional and a default summary will be displayed if not set.
      'summary' => t('The value will be wrapped in a paragraph tag.'),
      // The HTML tag to be passed to the html_tag element. This is required.
      'tag' => 'p',
      // An array of HTML attributed as passed to the html_tag element.
      // @see theme_html_tag().
      'attributes' => array(
        'class' => array('my-paragraph'),
      ),
    ),
  );
}

/**
 * Alters the array of wrappers available.
 *
 * @param array $wrappers
 *   An array containing all available wrappers.
 */
function hook_field_token_value_wrapper_info_alter(&$wrappers) {
  if (isset($wrappers['p'])) {
    // Add an ID attribute and update the summary.
    $wrappers['p']['attributes']['id'] = 'my-paragraph-id';
    $wrappers['p']['summary'] = t('Wrap the value in a paragraph with a custom ID attribute.');
  }
}

/**
 * Allows for the altering of the html_tag render array prior to rendering.
 *
 * @param array $element
 *   The html_tag render array to be altered.
 * @param array $wrapper_info
 *   An array containing the information for the wrapper to be used.
 */
function hook_field_token_value_output_alter(&$element, $wrapper_info) {
  // Attach a CSS file if the paragraph wrapper is being used.
  if ($wrapper_info['tag'] == 'p') {
    $element['#attached']['css'][] = drupal_get_path('module', 'my_module') . '/css/my-styles.css';
  }
}

/**
 * @} End of "addtogroup hooks".
 */
