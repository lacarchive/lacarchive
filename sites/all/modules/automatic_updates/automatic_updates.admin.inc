<?php

/**
 * @file
 * Administration functions for Automatic Updates module.
 */

/**
 * Form callback for administrator interface.
 */
function automatic_updates_admin_form() {
  $form['psa'] = [
    '#type' => 'fieldset',
    '#title' => t('Public service announcements'),
    '#collapsible' => TRUE,
  ];

  $form['psa']['description'] = [
    '#markup' => '<p>' . t('Public service announcements are compared against the entire code for the site, not just installed extensions.') . '</p>',
  ];
  $form['psa']['automatic_updates_enable_psa'] = [
    '#type' => 'checkbox',
    '#title' => t('Show Public service announcements on administrative pages.'),
    '#default_value' => variable_get('automatic_updates_enable_psa', TRUE),
  ];
  $form['psa']['automatic_updates_notify'] = [
    '#type' => 'checkbox',
    '#title' => t('Send email notifications for Public service announcements.'),
    '#default_value' => variable_get('automatic_updates_notify', TRUE),
    '#description' => t('The email addresses listed in <a href="@update_manager">update manager settings</a> will be notified.', ['@update_manager' => url('admin/reports/updates/settings')]),
  ];
  $last_check_timestamp = ReadinessCheckerManager::timestamp();
  $form['readiness'] = [
    '#type' => 'fieldset',
    '#title' => t('Readiness checks'),
    '#collapsible' => TRUE,
  ];

  $form['readiness']['automatic_updates_enable_readiness_checks'] = [
    '#type' => 'checkbox',
    '#title' => t('Check the readiness of automatically updating the site.'),
    '#default_value' => variable_get('automatic_updates_enable_readiness_checks', TRUE),
  ];
  if (ReadinessCheckerManager::isEnabled()) {
    $form['readiness']['automatic_updates_enable_readiness_checks']['#description'] = t('Readiness checks were last run @time ago. Manually <a href="@link">run the readiness checks</a>.', [
      '@time' => format_interval(REQUEST_TIME - $last_check_timestamp),
      '@link' => url('admin/config/system/automatic_updates/readiness'),
    ]);
  }
  $form['readiness']['automatic_updates_ignored_paths'] = [
    '#type' => 'textarea',
    '#title' => t('Paths to ignore for readiness checks'),
    '#description' => t('Paths relative to %drupal_root. One path per line. Automatic Updates is intentionally limited to Drupal core. It is recommended to ignore paths to contrib extensions.', ['%drupal_root' => DRUPAL_ROOT]),
    '#default_value' => variable_get('automatic_updates_ignored_paths', "sites/all/modules/*\nsites/all/themes/*"),
    '#states' => [
      'visible' => [
        ':input[name="automatic_updates_enable_readiness_checks"]' => ['checked' => TRUE],
      ],
    ],
  ];

  update_refresh();
  update_fetch_data();
  $available = update_get_available(TRUE);
  $projects = update_calculate_project_data($available);
  $not_recommended_version = $projects['drupal']['status'] !== UPDATE_CURRENT;
  $not_dev_core = strpos(VERSION, '-dev') === FALSE;
  $security_update = in_array($projects['drupal']['status'], [UPDATE_NOT_SECURE, UPDATE_REVOKED], TRUE);
  $recommended_release = $projects['drupal']['releases'][$projects['drupal']['recommended']];
  $major_upgrade = $recommended_release['version_major'] !== $projects['drupal']['existing_major'];
  $form['experimental'] = [
    '#type' => 'fieldset',
    '#title' => t('Experimental'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  ];
  if ($not_recommended_version && $not_dev_core) {
    if ($security_update) {
      $form['experimental']['security'] = [
        '#type' => 'html_tag',
        '#tag' => 'p',
        '#value' => t('A security update is available for your version of Drupal.'),
      ];
    }
    if ($major_upgrade) {
      $form['experimental']['major_version'] = [
        '#type' => 'html_tag',
        '#tag' => 'p',
        '#value' => t('This update is a major version update which means that it may not be backwards compatible with your currently running version. It is recommended that you read the release notes and proceed at your own risk.'),
      ];
    }
  }
  $update_text = t('Your site is running %version of Drupal core. No recommended update is available at this time.', ['%version' => VERSION]);
  if ($not_recommended_version && $not_dev_core) {
    $from_version = VERSION;
    $to_version = $recommended_release['version'];
    $query['token'] = drupal_get_token('in-place-automatic-updates-link');
    $update_text = t('Even with all that caution, if you want to try it out, <a href="@link">manually update now</a>.', [
      '@link' => url("/automatic_updates/in-place-update/drupal/core/$from_version/$to_version", ['query' => $query]),
    ]);
  }

  $form['experimental']['update'] = [
    '#prefix' => 'Upgrades with database updates are automatically <strong>rolled back</strong> for the integrity of the site. This module does not have a stable release and it is recommended to not use these features on a live website. Use at your own risk.',
    '#type' => 'html_tag',
    '#tag' => 'p',
    '#value' => $update_text,
  ];

  $form['experimental']['automatic_updates_enable_cron_updates'] = [
    '#type' => 'checkbox',
    '#title' => t('Enable automatic updates of Drupal core via cron.'),
    '#default_value' => variable_get('automatic_updates_enable_cron_updates', FALSE),
    '#description' => t('When a recommended update for Drupal core is available, a manual method to update is available. As an alternative to the full control of manually executing an update, enable automated updates via cron.'),
    '#states' => [
      'visible' => [
        ':input[name="automatic_updates_enable_readiness_checks"]' => ['checked' => TRUE],
      ],
    ],
  ];
  $form['experimental']['automatic_updates_enable_cron_security_updates'] = [
    '#type' => 'checkbox',
    '#title' => t('Enable security only updates'),
    '#default_value' => variable_get('automatic_updates_enable_cron_security_updates', FALSE),
    '#description' => t('Enable automated updates via cron for security-only releases of Drupal core.'),
    '#states' => [
      'visible' => [
        ':input[name="automatic_updates_enable_cron_updates"]' => ['checked' => TRUE],
      ],
    ],
  ];
  $form['actions']['#type'] = 'actions';
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save configuration'));
  return $form;
}

/**
 * Submit handler for automatic_updates_admin_form.
 */
function automatic_updates_admin_form_submit($form, &$form_state) {
  // Exclude unnecessary elements.
  form_state_values_clean($form_state);

  foreach ($form_state['values'] as $key => $value) {
    if (is_array($value) && isset($form_state['values']['array_filter'])) {
      $value = array_keys(array_filter($value));
    }
    variable_set($key, $value);
    // Disable cron automatic updates if readiness checks are disabled.
    if (in_array($key, ['automatic_updates_enable_cron_updates', 'automatic_updates_enable_cron_security_updates'], TRUE) && !$form_state['values']['automatic_updates_enable_readiness_checks']) {
      variable_set($key, FALSE);
    }
  }

  drupal_set_message(t('The configuration options have been saved.'));
}

/**
 * Page callback to run all checkers.
 */
function automatic_updates_run_checks() {
  $messages = [];
  foreach (ReadinessCheckerManager::getCategories() as $category) {
    $messages[] = ReadinessCheckerManager::run($category);
  }
  $messages = array_merge(...$messages);
  if (empty($messages)) {
    drupal_set_message(t('No issues found. Your site is ready for <a href="@readiness_checks">automatic updates</a>.', ['@readiness_checks' => 'https://www.drupal.org/docs/7/update/automatic-updates#readiness-checks']), 'status', FALSE);
  }
  drupal_goto('admin/config/system/automatic_updates');
}
