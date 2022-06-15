<?php

/**
 * @file
 * Defines upload callbacks of DFM Drupal plugin.
 */

/**
 * Implements plugin_hook_upload_file_validate().
 * Validates uploaded file.
 */
function dfm_drupal_upload_file_validate($dfm, $file, $existing) {
  $file->uri = $file->filepath;
  $uid = $dfm->getConf('drupalUid');
  // Check if the destination is in use. Track diff in filesize for quota validation.
  $diff = 0;
  if ($existing && $destfile = dfm_file_by_path($file->destination)) {
    if (!$dfm->getConf('ignoreFileUsage') && dfm_get_file_references($destfile)) {
      $dfm->setMessage(t('The destination %filename is in use by another application and can not be overwritten.', array('%filename' => $destfile->filename)));
      return FALSE;
    }
    // Take the overwritten size into account only if it belongs to the same user.
    if ($destfile->uid == $uid) {
      $diff -= $destfile->filesize;
    }
  }
  // Validate quota
  $file->uid = $uid;
  if ($error = dfm_get_quota_error($file, $dfm->getConf('uploadQuota'), $diff)) {
    $dfm->setMessage($error);
    return FALSE;
  }
  // Invoke hook_file_validate
  $file->status = 0;
  $file->filemime = file_get_mimetype($file->filename);
  if ($errors = module_invoke_all('file_validate', $file)) {
    foreach ($errors as $error) {
      $dfm->setMessage($error);
    }
    return FALSE;
  }
  return TRUE;
}

/**
 * Implements plugin_hook_upload_file().
 * Responds to a file upload.
 */
function dfm_drupal_upload_file($dfm, $file, $existing) {
  $file->uri = $file->filepath;
  $file->status = 1;
  $file->jsprops['date'] = $file->timestamp = time();
  dfm_write_file_record($file);
  // Include file id in response variables as it may be helpful to some apps like dfm_filefield.
  $file->jsprops['fid'] = $file->fid;
}