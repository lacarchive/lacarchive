<?php

/**
 * Implementation of hook_menu().
 */
function _dfm_menu() {
  $items = array();
  $items['dfm'] = array(
    'title' => 'File manager',
    'page callback' => 'dfm',
    'access callback' => 'dfm_access',
    'access arguments' => array(NULL, 1),
    'file' => 'dfm.page.inc',
    'type' => MENU_CALLBACK,
  );
  $items['admin/config/media/dfm'] = array(
    'title' => 'Drupella FM',
    'page callback' => 'dfm_admin',
    'access arguments' => array('administer Drupella FM'),
    'file' => 'dfm.admin.inc',
  );
  $items['admin/config/media/dfm/profile'] = array(
    'title' => 'Add new profile',
    'page callback' => 'dfm_profile_operations',
    'access arguments' => array('administer Drupella FM'),
    'type' => MENU_VISIBLE_IN_BREADCRUMB,
    'file' => 'dfm.admin.inc',
  );
  return $items;
}