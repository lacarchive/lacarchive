<?php

/**
 * Class PostUpdateNotify.
 */
class PostUpdateNotify {

  /**
   * Send notification on post update with success/failure.
   *
   * @param bool $success
   *   The update success status.
   * @param string $project
   *   The project name.
   * @param string $from_version
   *   The current project version.
   * @param string $to_version
   *   The desired next project version.
   */
  public static function send($success, $project, $from_version, $to_version) {
    $notify_list = variable_get('update_notify_emails', '');
    if (!empty($notify_list)) {
      $frequency = variable_get('automatic_updates_check_frequency', 43200);
      $last_check = variable_get('automatic_updates.notify_last_check', 0);
      variable_set('automatic_updates_last_check', REQUEST_TIME);
      $default_language = language_default();
      foreach ($notify_list as $target) {
        if ($target_user = user_load_by_mail($target)) {
          $target_language = user_preferred_language($target_user);
        }
        else {
          $target_language = $default_language;
        }
        $params['subject'] = t('Automatic update of "@project" succeeded', ['@project' => $project]);
        if (!$success) {
          $params['subject'] = t('Automatic update of "@project" failed', ['@project' => $project]);
        }
        $params['body'] = [
          '#theme' => 'automatic_updates_post_update',
          '#success' => $success,
          '#project' => $project,
          '#from_version' => $from_version,
          '#to_version' => $to_version,
        ];
        $params['langcode'] = $target_language->language;
        drupal_mail('automatic_updates', 'notify', $target, $params['langcode'], $params);
      }
    }
  }

}
