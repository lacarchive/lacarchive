<?php

/**
 * File ownership checker.
 */
class FileOwnership implements ReadinessCheckerInterface {

  /**
   * {@inheritdoc}
   */
  public static function run() {
    $file_path = DRUPAL_ROOT . '/includes/bootstrap.inc';
    return static::ownerIsScriptUser($file_path);
  }

  /**
   * Check if file is owned by the same user as which is running the script.
   *
   * Helps identify scenarios when the check is run by web user and the files
   * are owned by a non-web user.
   *
   * @param string $file_path
   *   The file path to check.
   *
   * @return array
   *   An array of translatable strings if there are file ownership issues.
   */
  protected static function ownerIsScriptUser($file_path) {
    $messages = [];
    if (function_exists('posix_getuid')) {
      $file_owner_uid = fileowner($file_path);
      $script_uid = posix_getuid();
      if ($file_owner_uid !== $script_uid) {
        $messages[] = t('Files are owned by uid "@owner" but PHP is running as uid "@actual". The file owner and PHP user should be the same during an update.', [
          '@owner' => $file_owner_uid,
          '@file' => $file_path,
          '@actual' => $script_uid,
        ]);
      }
    }
    return $messages;
  }

}
