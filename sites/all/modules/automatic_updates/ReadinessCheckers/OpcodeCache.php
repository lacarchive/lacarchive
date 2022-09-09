<?php

/**
 * Error if opcode caching is enabled and updates are executed via CLI.
 */
class OpcodeCache implements ReadinessCheckerInterface {

  /**
   * {@inheritdoc}
   */
  public static function run() {
    $messages = [];
    if (self::isCli() && self::hasOpcodeFileCache()) {
      $messages[] = t('Automatic updates cannot run via CLI  when opcode file cache is enabled.');
    }
    return $messages;
  }

  /**
   * Determine if PHP is running via CLI.
   *
   * @return bool
   *   TRUE if CLI, FALSE otherwise.
   */
  protected static function isCli() {
    return PHP_SAPI === 'cli';
  }

  /**
   * Determine if opcode cache is enabled.
   *
   * If opcache.validate_timestamps is disabled or enabled with
   * opcache.revalidate_freq greater then 2, then a site is considered to have
   * opcode caching. The default php.ini setup is
   * opcache.validate_timestamps=TRUE and opcache.revalidate_freq=2.
   *
   * @return bool
   *   TRUE if opcode file cache is enabled, FALSE otherwise.
   */
  protected static function hasOpcodeFileCache() {
    if (!ini_get('opcache.validate_timestamps')) {
      return TRUE;
    }
    if (ini_get('opcache.revalidate_freq') > 2) {
      return TRUE;
    }
    return FALSE;
  }

}
