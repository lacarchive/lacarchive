<?php

/**
 * Implementation of hook_theme().
 */
function _dfm_theme() {
  $theme['dfm_admin_form'] = array('function' => 'dfm_admin_form_theme', 'render element' => 'form');
  $theme['dfm_directory_form'] = array('function' => 'dfm_directory_form_theme', 'render element' => 'form');
  $theme['dfm_page'] = array(
    'template' => 'dfm-page',
    'variables' => array('page' => NULL),
    'file' => 'dfm.page.inc',
  );
  return $theme;
}