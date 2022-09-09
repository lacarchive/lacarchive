<?php

/**
 * Warn if PHP SAPI changes between checker executions.
 */
class PhpSapi implements ReadinessCheckerInterface {

  /**
   * {@inheritdoc}
   */
  public static function run() {
    $messages = [];
    $php_sapi = variable_get('automatic_updates_php_sapi', PHP_SAPI);
    if ($php_sapi !== PHP_SAPI) {
      $messages[] = t('PHP changed from running as "@previous" to "@current". This can lead to inconsistent and misleading results.', ['@previous' => $php_sapi, '@current' => PHP_SAPI]);
    }
    variable_set('automatic_updates.php_sapi', PHP_SAPI);
    return $messages;
  }

}
