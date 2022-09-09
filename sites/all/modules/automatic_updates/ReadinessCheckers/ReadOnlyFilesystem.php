<?php

/**
 * Read only filesystem checker.
 */
class ReadOnlyFilesystem implements ReadinessCheckerInterface {

  /**
   * {@inheritdoc}
   */
  public static function run() {
    return static::readOnlyCheck();
  }

  /**
   * Check if the filesystem is read only.
   *
   * @return array
   *   An array of translatable strings if any checks fail.
   */
  protected  static function readOnlyCheck() {
    $messages = [];

    // If we can copy and delete a file, then we don't have a read only
    // file system.
    $file_path = DRUPAL_ROOT;
    $file = 'modules/node/node.api.php';
    if (file_unmanaged_copy("$file_path/$file", "$file_path/$file.automatic_updates", FILE_EXISTS_REPLACE)) {
      // Delete it after copying.
      file_unmanaged_delete("$file_path/$file.automatic_updates");
    }
    else {
      $error = t('Filesystem at "@path" is read only. Updates to Drupal core cannot be applied against a read only file system.', ['@path' => DRUPAL_ROOT]);
      watchdog('automatic_updates', $error, [], WATCHDOG_ERROR);
      $messages[] = $error;
    }
    return $messages;
  }

}
