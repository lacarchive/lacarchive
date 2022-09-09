<?php

/**
 * Cron frequency checker.
 */
class CronFrequency implements ReadinessCheckerInterface {

  /**
   * Minimum cron threshold is 3 hours.
   */
  const MINIMUM_CRON_INTERVAL = 10800;

  /**
   * {@inheritdoc}
   */
  public static function run() {
    $messages = [];
    if (variable_get('cron_safe_threshold', DRUPAL_CRON_DEFAULT_THRESHOLD) > static::MINIMUM_CRON_INTERVAL) {
      $messages[] = t('Cron is not set to run frequently enough. <a href="@configure">Configure it</a> to run at least every 3 hours or disable automated cron and run it via an external scheduling system.', [
        '@configure' => url('admin/config/system/cron'),
      ]);
    }
    // Determine when cron last ran.
    $cron_last = variable_get('cron_last');
    if (!is_numeric($cron_last)) {
      $cron_last = variable_get('install_time', 0);
    }
    if (REQUEST_TIME - $cron_last > static::MINIMUM_CRON_INTERVAL) {
      $messages[] = t('Cron has not run recently. <a href="@configure">Configure it</a> to run at least every 3 hours or disable automated cron and run it via an external scheduling system.', [
        '@configure' => url('admin/config/system/cron'),
      ]);
    }
    return $messages;
  }

}
