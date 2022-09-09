<?php

/**
 * Class Notify.
 */
class Notify {

  /**
   * Send notification for PSAs.
   */
  public static function send() {
    // Don't send mail if notifications are disabled.
    if (!variable_get('automatic_updates_notify', TRUE)) {
      return;
    }
    $messages = AutomaticUpdatesPsa::getPublicServiceMessages();
    if (!$messages) {
      return;
    }
    $notify_list = variable_get('update_notify_emails', '');
    if (!empty($notify_list)) {
      $frequency = variable_get('automatic_updates_check_frequency', 43200);
      $last_check = variable_get('automatic_updates.notify_last_check', 0);
      if ((REQUEST_TIME - $last_check) > $frequency) {
        variable_set('automatic_updates_last_check', REQUEST_TIME);
        $default_language = language_default();
        foreach ($notify_list as $target) {
          if ($target_user = user_load_by_mail($target)) {
            $target_language = user_preferred_language($target_user);
          }
          else {
            $target_language = $default_language;
          }
          $params['subject'] = format_plural(
            count($messages),
            '@count urgent Drupal announcement requires your attention for @site_name',
            '@count urgent Drupal announcements require your attention for @site_name',
            ['@site_name' => variable_get('site_name', 'Drupal')],
            ['langcode' => $target_language->language]
          );
          $params['body'] = [
            '#theme' => 'automatic_updates_psa_notify',
            '#messages' => $messages,
          ];
          $params['langcode'] = $target_language->language;
          drupal_mail('automatic_updates', 'notify', $target, $params['langcode'], $params);
        }
      }
    }
  }

}
