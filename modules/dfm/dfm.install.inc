<?php

/**
 * Installs default profiles.
 */
function dfm_install_default_profiles() {
  $admin_conf = dfm_default_admin_profile_conf();
  $member_conf = dfm_default_member_profile_conf();
  db_query("INSERT INTO {dfm_profiles} (name, conf) VALUES (:name, :conf)", array(':name' => 'Administrators', ':conf' => serialize($admin_conf)));
  db_query("INSERT INTO {dfm_profiles} (name, conf) VALUES (:name, :conf)", array(':name' => 'Members', ':conf' => serialize($member_conf)));
}

/**
 * Returns admin profile.
 */
function dfm_default_admin_profile_conf() {
  return array(
    'uploadMaxSize' => 0,
    'uploadQuota' => 0,
    'uploadExtensions' => '*',
    'imgMaxDim' => 0,
    'searchOn' => 1,
    'searchLimit' => 0,
    'dirConfRaw' => array(
      array(
        'dirname' => '.',
        'perms' => array('all' => 1),
        'subdirConf' => array('inherit' => 1),
      ),
    ),
  );
}

/**
 * Returns member profile.
 */
function dfm_default_member_profile_conf() {
  return array(
    'uploadMaxSize' => 2,
    'uploadQuota' => 20,
    'uploadExtensions' => 'jpg png gif',
    'imgMaxDim' => '800x800',
    'searchOn' => 1,
    'searchLimit' => 100,
    'dirConfRaw' => array(
      array(
        'dirname' => 'users/user[user:uid]',
        'perms' => array('listFiles' => 1, 'uploadFiles' => 1, 'deleteFiles' => 1),
      ),
    ),
  );
}

/**
 * Updates directory permissions of a configuration profile.
 * Used by dfm plugins on install.
 */
function dfm_update_profile_dir_perms($pid, $dir, array $perms) {
  if ($profile = dfm_load_profile($pid)) {
    $update = FALSE;
    foreach ($profile->conf['dirConfRaw'] as &$dirconf) {
      if (!empty($dirconf['perms']) && ($dir === TRUE || $dir === $dirconf['dirname'])) {
        foreach ($perms as $perm => $value) {
          // Enable
          if ($value) {
            if (empty($dirconf['perms'][$perm])) {
              $dirconf['perms'][$perm] = $value;
              $update = TRUE;
            }
          }
          // Disable
          elseif (isset($dirconf['perms'][$perm])) {
            unset($dirconf['perms'][$perm]);
            $update = TRUE;
          }
        }
      }
    }
    if ($update) {
      db_query('UPDATE {dfm_profiles} SET conf = :conf WHERE pid = :pid', array(':conf' => serialize($profile->conf), ':pid' => $pid));
    }
    return $update;
  }
}

/**
 * Updates values of a configuration profile.
 * Used by dfm plugins on install.
 */
function dfm_update_profile_settings($pid, array $settings) {
  if ($profile = dfm_load_profile($pid)) {
    $update = FALSE;
    foreach ($settings as $key => $value) {
      if (isset($profile->conf[$key]) && $value === NULL) {
        unset($profile->conf[$key]);
        $update = TRUE;
      }
      elseif (!isset($profile->conf[$key]) || $value !== $profile->conf[$key]) {
        $profile->conf[$key] = $value;
        $update = TRUE;
      }
    }
    if ($update) {
      db_query('UPDATE {dfm_profiles} SET conf = :conf WHERE pid = :pid', array(':conf' => serialize($profile->conf), ':pid' => $pid));
    }
    return $update;
  }
}
