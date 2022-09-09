<?php

/**
 * @file
 * Automatic updates page callback file for the automatic updates module.
 */

/**
 * Menu callback; process in place updates.
 */
function automatic_updates_in_place_update($project, $type, $from, $to) {
  if (!isset($_GET['token']) || !drupal_valid_token($_GET['token'], 'in-place-automatic-updates-link')) {
    return MENU_ACCESS_DENIED;
  }
  $updated = InPlaceUpdate::update($project, $type, $from, $to);
  $message_type = 'status';
  $message = t('Update successful');
  if (!$updated) {
    $message_type = 'error';
    $message = t('Update Failed');
  }
  drupal_set_message($message, $message_type);
  drupal_goto('admin/config/system/automatic_updates');
}
