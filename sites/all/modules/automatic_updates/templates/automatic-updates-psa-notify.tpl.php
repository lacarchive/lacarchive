<?php

/**
 * @file
 * Template for the public service announcements email notification.
 *
 * Available variables:
 * - $messages: The messages array.
 *
 * @ingroup themeable
 */
?>
<p>
  <?php print t('A security update will be made available soon for your Drupal site. To ensure the security of the site, you should prepare the site to immediately apply the update once it is released!'); ?>
</p>
<p>
  <?php $status_report = url('admin/reports/status', ['absolute' => TRUE])?>
  <?php print t('See the <a href="!status_report">site status report page</a> for more information.', ['!status_report' => $status_report]); ?>
</p>
<p><?php print t('Public service announcements:'); ?></p>
<ul>
  <?php foreach ($messages as $message): ?>
    <li><?php print $message; ?></li>
  <?php endforeach; ?>
</ul>
<p><?php print t('To see all PSAs, visit <a href="!url">!url</a>.', ['!url' => 'https://www.drupal.org/security/psa']); ?></p>
<p>
  <?php $settings_link = url('admin/config/system/automatic_updates', ['absolute' => TRUE]); ?>
  <?php print t('Your site is currently configured to send these emails when a security update will be made available soon. To change how you are notified, you may <a href="!settings_link">configure email notifications</a>.', ['!settings_link' => $settings_link]); ?>
</p>
