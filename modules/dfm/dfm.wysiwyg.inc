<?php

/**
 * @file
 * Defines callbacks for WYSIWYG integration.
 */

/**
 * Implements hook_wysiwyg_plugin().
 */
function _dfm_wysiwyg_plugin($editor, $version) {
  $plugins = array();
  $dfm = $dfmButtons = array();
  // TinyMCE
  if ($editor === 'tinymce') {
    $dfm['options']['file_browser_callback'] = 'dfmWysiwyg.mceBrowser';
    // Buttons
    $dfmButtons['path'] = drupal_get_path('module', 'dfm') . '/wysiwyg_plugins/tinymce';
    $dfmButtons['filename'] = 'editor_plugin.js';
  }
  // CKEditor
  elseif ($editor === 'ckeditor') {
    // This will be corrected later in _dfm_wysiwyg_editor_settings_alter()
    $dfm['options']['filebrowserBrowseUrl'] = '/dfm';
    // Buttons
    $dfmButtons['path'] = drupal_get_path('module', 'dfm') . '/wysiwyg_plugins/ckeditor';
    $dfmButtons['filename'] = 'plugin.js';
  }
  // Add the plugins
  if ($dfm) {
    $dfm['extensions'] = array('dfm' => t('Drupella FM: Integrate into image/link dialogs'));
    $dfm['load'] = FALSE;
    $plugins['dfm'] = $dfm;
  }
  if ($dfmButtons) {
    $dfmButtons['buttons']['DfmImage'] = t('Drupella FM: Button for inserting multiple images');
    $dfmButtons['buttons']['DfmFile'] = t('Drupella FM: Button for inserting multiple file links');
    $dfmButtons['load'] = TRUE;
    $plugins['dfmButtons'] = $dfmButtons;
  }
  return $plugins;
}

/**
 * Implements hook_ckeditor_plugin().
 */
function _dfm_ckeditor_plugin() {
  $path = drupal_get_path('module', 'dfm');
  $plugins['dfmButtons'] = array(
    'name' => 'dfmButtons',
    'desc' => t('Drupella FM: Buttons for inserting multiple images and file links'),
    'path' => $path . '/wysiwyg_plugins/ckeditor/',
    'buttons' => array(
      'DfmImage' => array(
        'label' => 'Insert images',
        'icon' => 'image.png',
      ),
      'DfmFile' => array(
        'label' => 'Insert files',
        'icon' => 'file.png',
      ),
    ),
  );
  return $plugins;
}

/**
 * Implements hook_wysiwyg_editor_settings_alter().
 */
function _dfm_wysiwyg_editor_settings_alter(&$settings, $context) {
  // Tinymce
  if (isset($settings['file_browser_callback']) && $settings['file_browser_callback'] == 'dfmWysiwyg.mceBrowser') {
    if (!dfm_wysiwyg_integrate()) {
      unset($settings['file_browser_callback']);
    }
  }
  // Ckeditor
  elseif (isset($settings['filebrowserBrowseUrl']) && $settings['filebrowserBrowseUrl'] == '/dfm') {
    if (dfm_wysiwyg_integrate()) {
      $settings['filebrowserBrowseUrl'] = url('dfm', array('query' => array('fileHandler' => 'dfmWysiwyg.ckeHandler')));
    }
    else {
      unset($settings['filebrowserBrowseUrl']);
    }
  }
}

/**
 * Processes a textarea.
 */
function _dfm_textarea_proc($element) {
  static $regexp, $jsdone;
  if (!isset($regexp)) {
    if ($regexp = str_replace(' ', '', variable_get('dfm_textareas', ''))) {
      $regexp = '@^(' . str_replace(',', '|', implode('.*', array_map('preg_quote', explode('*', $regexp)))) . ')$@';
    }
  }
  // Inline textarea integration
  if ($regexp && preg_match($regexp, $element['#id'])) {
    drupal_add_js(array('dfmTextareas' => array($element['#id'])), 'setting');
    if (!isset($jsdone)) {
      $jsdone = dfm_wysiwyg_integrate();
    }
  }
  // CKEditor integration. Need to check if filebrowserBrowseUrl is set as dfm path under custom conf
  // but there is no sane way to do that. We include the script just in case.
  else if (!isset($jsdone)) {
    if (module_exists('ckeditor')) {
      $jsdone = dfm_wysiwyg_integrate();
    }
  }
  return $element;
}