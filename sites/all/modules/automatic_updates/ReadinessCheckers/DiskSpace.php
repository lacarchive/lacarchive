<?php

/**
 * Disk space checker.
 */
class DiskSpace implements ReadinessCheckerInterface {

  /**
   * Minimum disk space (in bytes) is 10mb.
   */
  const MINIMUM_DISK_SPACE = 10000000;

  /**
   * Megabyte divisor.
   */
  const MEGABYTE_DIVISOR = 1000000;

  /**
   * {@inheritdoc}
   */
  public static function run() {
    return static::diskSpaceCheck();
  }

  /**
   * Check if the filesystem has sufficient disk space.
   *
   * @return array
   *   An array of translatable strings if there is not sufficient space.
   */
  protected static function diskSpaceCheck() {
    $messages = [];
    if (disk_free_space(DRUPAL_ROOT) < static::MINIMUM_DISK_SPACE) {
      $messages[] = t('Logical disk "@root" has insufficient space. There must be at least @space megabytes free.', [
        '@root' => DRUPAL_ROOT,
        '@space' => static::MINIMUM_DISK_SPACE / static::MEGABYTE_DIVISOR,
      ]);
    }
    $temp = drupal_realpath('temporary://');
    if (disk_free_space($temp) < static::MINIMUM_DISK_SPACE) {
      $messages[] = t('Directory "@temp" has insufficient space. There must be at least @space megabytes free.', [
        '@temp' => $temp,
        '@space' => static::MINIMUM_DISK_SPACE / static::MEGABYTE_DIVISOR,
      ]);
    }

    return $messages;
  }

}
