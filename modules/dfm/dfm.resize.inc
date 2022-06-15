<?php

/**
 * @file
 * Defines resize callbacks of DFM Drupal plugin.
 */

/**
 * Implements plugin_hook_resize_filepath_validate().
 */
function dfm_drupal_resize_filepath_validate($dfm, $filepath, $newpath, $params) {
  // Check if the destination is in use.
  if ($params['existing'] && !$dfm->getConf('ignoreFileUsage') && $destfile = dfm_file_by_path($newpath)) {
    if (dfm_get_file_references($destfile)) {
      $dfm->setMessage(t('The destination %filename is in use by another application and can not be overwritten.', array('%filename' => $destfile->filename)));
      return FALSE;
    }
  }
  return TRUE;
}

/**
 * Implements plugin_hook_resize_filepath().
 * Responds to a file resize.
 */
function dfm_drupal_resize_filepath($dfm, $filepath, $newpath, $params) {
  // Update existing record
  if ($params['existing']) {
    // Update existing record.
    if ($file = dfm_file_by_path($newpath)) {
      db_query("UPDATE {file_managed} SET filesize = :filesize WHERE fid = :fid", array(':filesize' => filesize($newpath), ':fid' => $file->fid));
    }
    // We do not create a new db record for existing files.
  }
  // Create new db record for the new file.
  else {
    $file = (object) array(
      'uid' => $dfm->getConf('drupalUid'),
      'filename' => $dfm->basename($newpath),
      'uri' => $newpath,
      'filesize' => filesize($newpath),
      'filemime' => file_get_mimetype($newpath),
      'timestamp' => time(),
      'status' => 1,
      'dfm' => 1,
    );
    dfm_write_file_record($file, TRUE);
  }
}