<?php

/**
 * @file
 * Defines copy callbacks of DFM Drupal plugin.
 */

/**
 * Implements plugin_hook_copy_filepath_validate().
 * Checks if the destination can be overwritten and validates quota.
 */
function dfm_drupal_copy_filepath_validate($dfm, $filepath, $newpath, $existing) {
  $uid = $dfm->getConf('drupalUid');
  // Check if the destination is in use. Track diff in filesize for quota validation.
  $diff = 0;
  if ($existing && $destfile = dfm_file_by_path($newpath)) {
    if (!$dfm->getConf('ignoreFileUsage') && dfm_get_file_references($destfile)) {
      $dfm->setMessage(t('The destination %filename is in use by another application and can not be overwritten.', array('%filename' => $destfile->filename)));
      return FALSE;
    }
    // Take the overwritten size into account only if it belongs to the same user.
    if ($destfile->uid == $uid) {
      $diff -= $destfile->filesize;
    }
  }
  // Skip all validations except quota as this is a copy operation.
  // Create file object.
  $file = (object) array(
    'uid' => $uid,
    'filename' => $dfm->basename($newpath),
    'filesize' => filesize($filepath),
  );
  // Validate quota
  if ($error = dfm_get_quota_error($file, $dfm->getConf('uploadQuota'), $diff)) {
    $dfm->setMessage($error);
    return FALSE;
  }
  return TRUE;
}

/**
 * Implements plugin_hook_copy_filepath().
 * Responds to a file copy.
 */
function dfm_drupal_copy_filepath($dfm, $filepath, $newpath, $existing) {
  // Create the copied file's record.
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
  dfm_write_file_record($file);
}