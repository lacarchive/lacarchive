<?php

/**
 * Pending database updates checker.
 */
class PendingDbUpdates implements ReadinessCheckerInterface {

  /**
   * {@inheritdoc}
   */
  public static function run() {
    $messages = [];

    if (static::areDbUpdatesPending()) {
      $messages[] = t('There are pending database updates. Please run update.php.');
    }
    return $messages;
  }

  /**
   * Checks if there are pending database updates.
   *
   * @return bool
   *   TRUE if there are pending updates, otherwise FALSE.
   */
  protected static function areDbUpdatesPending() {
    require_once DRUPAL_ROOT . '/includes/install.inc';
    require_once DRUPAL_ROOT . '/includes/update.inc';
    drupal_load_updates();
    return (bool) update_get_update_list();
  }

}
