<?php

/**
 * Modified code checker.
 */
class ModifiedFiles implements ReadinessCheckerInterface {
  use ProjectInfoTrait;

  /**
   * {@inheritdoc}
   */
  public static function run() {
    return static::modifiedFilesCheck();
  }

  /**
   * Check if the site contains any modified code.
   *
   * @return array
   *   An array of translatable strings if any checks fail.
   */
  protected static function modifiedFilesCheck() {
    $messages = [];
    $filtered_modified_files = new IgnoredPathsIteratorFilter(ModifiedFilesService::getModifiedFiles(static::getInfos()));
    foreach ($filtered_modified_files as $file) {
      $messages[] = t('The hash for @file does not match its original. Updates that include that file will fail and require manual intervention.', ['@file' => $file]);
    }
    return $messages;
  }

}
