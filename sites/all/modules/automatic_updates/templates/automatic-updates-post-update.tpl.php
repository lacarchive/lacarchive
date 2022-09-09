<?php

/**
 * @file
 * Template for the public service announcements email notification.
 *
 * Available variables:
 * - $success: The update success status.
 * - $project: The project name.
 * - $from_version: The current project version.
 * - $to_version: The desired next project version.
 *
 * @ingroup themeable
 */
?>
<p>
  <?php if ($success): ?>
    <?php print t('The project "@project" was updated from "@from_version" to "@to_version" with success.', [
      '@project' => $project,
      '@from_version' => $from_version,
      '@to_version' => $to_version,
    ]); ?>
  <?php else: ?>
    <?php print t('The project "@project" was updated from "@from_version" to "@to_version" with failures.', [
      '@project' => $project,
      '@from_version' => $from_version,
      '@to_version' => $to_version,
    ]); ?>
  <?php endif; ?>
</p>
<p>
  <?php $status_report = url('admin/reports/status', ['absolute' => TRUE])?>
  <?php print t('See the <a href="!status_report">site status report page</a> and any logs for more information.', ['!status_report' => $status_report]); ?>
</p>
