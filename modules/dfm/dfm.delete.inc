<?php

/**
 * @file
 * Defines delete callbacks of DFM Drupal plugin.
 */

/**
 * Implements plugin_hook_delete_filepath_validate().
 * Checks if a file can be deleted.
 */
function dfm_drupal_delete_filepath_validate($dfm, $filepath) {
  if (!$dfm->getConf('ignoreFileUsage') && ($file = dfm_file_by_path($filepath)) && dfm_get_file_references($file)) {
    $dfm->setMessage(t('%filename is in use by another application.', array('%filename' => $file->filename)));
    return FALSE;
  }
  return TRUE;
}

/**
 * Implements plugin_hook_delete_filepath().
 * Responds to a file deletion.
 */
function dfm_drupal_delete_filepath($dfm, $filepath) {
  dfm_delete_filepath_record($filepath);
}