<?php

use Composer\Semver\VersionParser;

/**
 * Blacklisted PHP 7.2 version checker.
 */
class BlacklistPhp72Versions implements ReadinessCheckerInterface {

  /**
   * {@inheritdoc}
   */
  public static function run() {
    $project_root = drupal_get_path('module', 'automatic_updates');
    require_once $project_root . DIRECTORY_SEPARATOR . 'vendor' . DIRECTORY_SEPARATOR . 'autoload.php';
    $messages = [];
    $parser = new VersionParser();
    $unsupported_constraint = static::getUnsupportedVersionConstraint();
    if ($unsupported_constraint->matches($parser->parseConstraints(static::getPhpVersion()))) {
      $messages[] = static::getMessage();
    }
    return $messages;
  }

  /**
   * Get the PHP version.
   *
   * @return string
   *   The current php version.
   */
  protected static function getPhpVersion() {
    return PHP_MAJOR_VERSION . '.' . PHP_MINOR_VERSION . '.' . PHP_RELEASE_VERSION;
  }

  /**
   * {@inheritdoc}
   */
  protected static function getUnsupportedVersionConstraint() {
    $parser = new VersionParser();
    // Rather than make things complicated with cli vs non-cli PHP and
    // differences in their support of opcache, libsodium and Sodium_Compat,
    // simply blacklist the entire version range to ensure the best possible
    // and coherent update support.
    return $parser->parseConstraints('>=7.2.0 <=7.2.2');
  }

  /**
   * {@inheritdoc}
   */
  protected static function getMessage() {
    return t('PHP 7.2.0, 7.2.1 and 7.2.2 have issues with opcache that breaks signature validation. Please upgrade to a newer version of PHP to ensure assurance and security for package signing.');
  }

}
