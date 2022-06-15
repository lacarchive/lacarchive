<?php

/**
 * @file
 * Implements Dfm page callbacks.
 */

/**
 * Main handler for dfm path.
 */
function dfm($scheme = NULL) {
  $GLOBALS['devel_shutdown'] = FALSE;
  if (empty($_REQUEST['ajaxOp'])) {
    return dfm_page();
  }
  if ($fm = dfm_user_fm($GLOBALS['user'], $scheme)) {
    return $fm->run();
  }
}

/**
 * Prints Dfm page with the given page variables and exists.
 */
function dfm_page($page = array()) {
  drupal_add_http_header('Content-Type', 'text/html; charset=utf-8');
  print theme('dfm_page', array('page' => $page));
  drupal_page_footer();
  exit();
}

/**
 * Processes the given profile and returns the processed copy.
 */
function dfm_process_profile($profile) {
  // Check if already processed.
  if (!isset($profile->conf['dirConfRaw'])) {
    return $profile;
  }
  // Work on a copy
  $profile = clone($profile);
  $user = isset($profile->uid) && $profile->uid != $GLOBALS['user']->uid ? user_load($profile->uid) : $GLOBALS['user'];
  $profile->uid = $user->uid;
  if (!isset($profile->scheme)) $profile->scheme = file_default_scheme();
  // Run preprocessors
  foreach (variable_get('dfm_preprocess_profile', array()) as $func => $state) {
    if ($state && function_exists($func)) {
      $func($profile, $user);
    }
  }
  $conf = &$profile->conf;
  $conf['pid'] = $profile->pid;
  $conf['scheme'] = $profile->scheme;
  // Convert to bytes
  $conf['uploadMaxSize'] *= 1048576;
  $conf['uploadQuota'] *= 1048576;
  // Set extensions
  $conf['uploadExtensions'] = array_values(array_filter(explode(' ', $conf['uploadExtensions'])));
  if (isset($conf['imgExtensions'])) {
    $conf['imgExtensions'] = $conf['imgExtensions'] ? array_values(array_filter(explode(' ', $conf['imgExtensions']))) : FALSE;
  }
  // No need to set empty variables that default to false.
  if ($val = variable_get('allow_insecure_uploads', 0)) {
    $conf['uploadInsecure'] = $val;
  }
  if ($val = variable_get('dfm_abs_urls', 0)) {
    $conf['absUrls'] = $val;
  }
  // Set paths/urls
  $conf['rootDirPath'] = $conf['scheme'] . '://';
  $conf['rootDirUrl'] = dfm_root_url($conf['rootDirPath'], !empty($conf['absUrls']));
  $conf['scriptDirPath'] = dfm_script_path();
  $conf['baseUrl'] = base_path();
  // Add drupal plugin to call dfm_drupal_plugin_register().
  $conf['plugins'][] = 'drupal';
  // Set custom realpath function
  $conf['realpathFunc'] = 'drupal_realpath';
  $conf['securityKey'] = $GLOBALS['user']->uid ? drupal_get_token('dfm') : 'anonymous';
  $conf['jsCssSuffix'] = variable_get('css_js_query_string', '0');
  $conf['drupalUid'] = $user->uid;
  $conf['fileMode'] = variable_get('file_chmod_file', 0664);
  $conf['directoryMode'] = variable_get('file_chmod_directory', 0775);
  $conf['imgJpegQuality'] = variable_get('image_jpeg_quality', 85);
  $conf['lang'] = $GLOBALS['language']->language;
  // Set thumbnail URL
  if (!empty($conf['thumbStyle']) && function_exists('image_styles')) {
    $menupath = $conf['scheme'] === 'private' ? 'system/files' : file_stream_wrapper_get_instance_by_scheme('public')->getDirectoryPath();
    $conf['thumbUrl'] = url($menupath) . '/styles/' . $conf['thumbStyle'] . '/' . $conf['scheme'];
    if (!variable_get('image_allow_insecure_derivatives', FALSE)) {
      $conf['thumbUrlQuery'] = 'dfm_itok=' . dfm_itok($conf['thumbStyle']);
    }
  }
  // Process folders.
  $conf['dirConf'] = dfm_process_profile_folders($profile, $user, variable_get('dfm_merge_folders', 0));
  // Run postprocessors
  foreach (variable_get('dfm_postprocess_profile', array()) as $func => $state) {
    if ($state && function_exists($func)) {
      $func($profile, $user);
    }
  }
  drupal_alter('dfm_conf', $conf, $user);
  unset($conf['dirConfRaw']);
  // Apply chroot jail.
  if (!empty($conf['chrootJail'])) {
    dfm_chroot_jail($conf);
  }
  // $conf['displaySystemErrors'] = 1; // No need to set this as errors are handled by drupal_error_handler.
  // $conf['exposeRootDirPath'] = 1; // Display dirpaths as full path in UI status bar
  return $profile;
}

/**
 * Processes profile folders.
 */
function dfm_process_profile_folders($profile, $user, $merge = FALSE) {
  // Process folders.
  $ret = dfm_process_user_folders($profile->conf['dirConfRaw'], $user);
  // Merge folders from multiple profiles
  if ($merge) {
    $key = $profile->scheme . '_pid';
    foreach (variable_get('dfm_roles_profiles', array()) as $rid => $pids) {
      if (isset($user->roles[$rid]) && !empty($pids[$key]) && $pids[$key] != $profile->pid) {
        if ($_profile = dfm_load_profile($pids[$key])) {
          foreach (dfm_process_user_folders($_profile->conf['dirConfRaw'], $user) as $dirname => $dirconf) {
            // Existing folder
            if (isset($ret[$dirname])) {
              continue;
            }
            // Inherited folder
            foreach ($ret as $parent => $parent_conf) {
              if (!empty($parent_conf['subdirConf']['inherit']) && ($parent === '.' || strpos($dirname . '/', $parent . '/') === 0)) {
                continue 2;
              }
            }
            $ret[$dirname] = $dirconf;
          }
        }
      }
    }
  }
  return $ret;
}

/**
 * Processes user folders.
 */
function dfm_process_user_folders($folders, $user) {
  $ret = array();
  $data = array('user' => $user);
  foreach ($folders as $folder) {
    $dirname = $folder['dirname'];
    // Replace tokens
    if (strpos($dirname, '[') !== FALSE) {
      $dirname = token_replace($dirname, $data);
      // Unable to resolve token
      if ($dirname === $folder['dirname'] && strpos($dirname, ':')) {
        // Show message on admin UI
        if (strpos($_GET['q'], 'admin/config/media/dfm') === 0) {
          drupal_set_message(t('Unable to resolve token in folder name %name', array('%name' => $dirname)), 'warning');
        }
        continue;
      }
    }
    if ($dirname != '' && !isset($ret[$dirname])) {
      $ret[$dirname] = $folder;
      unset($ret[$dirname]['dirname']);
    }
  }
  return $ret;
}

/**
 * Apply chroot if there is a top-most directory.
 */
function dfm_chroot_jail(&$conf) {
  if (isset($conf['dirConf']['.'])) return;
  // Set the first one as topdir.
  $dirnames = array_keys($conf['dirConf']);
  $topdir = array_shift($dirnames);
  // Check the rest
  foreach ($dirnames as $dirname) {
    // This is a subdirectory of the topdir. No change.
    if (strpos($dirname . '/', $topdir . '/') === 0) {
      continue;
    }
    // This is a parent directory of the topdir. Make it the topdir.
    if (strpos($topdir . '/', $dirname . '/') === 0) {
      $topdir = $dirname;
      continue;
    }
    // Not a part of the same branch with topdir which means there is no top-most directory
    return;
  }
  // Create the new dir conf starting from the top
  $newdirconf['.'] = $conf['dirConf'][$topdir];
  unset($conf['dirConf'][$topdir]);
  // Add the rest
  $pos = strlen($topdir) + 1;
  foreach ($conf['dirConf'] as $dirname => $set) {
    $newdirconf[substr($dirname, $pos)] = $set;
  }
  $conf['dirConf'] = $newdirconf;
  $conf['rootDirPath'] .= (substr($conf['rootDirPath'], -1) == '/' ? '' : '/') . $topdir;
  $topdirurl = dfm_encode_path($topdir);
  $conf['rootDirUrl'] .= '/' . $topdirurl;
  // Also alter thumnail prefix
  if (isset($conf['thumbUrl'])) {
    $conf['thumbUrl'] .= '/' . $topdirurl;
  }
  return $topdir;
}

/**
 * Implements dfm_PLUGIN_plugin_register() for drupal plugin.
 */
function dfm_drupal_plugin_register($dfm) {
  $path = drupal_get_path('module', 'dfm');
  // Set a pre load callback
  $dfm->registerHook('ajax_load', 'dfm_drupal_pre_ajax_load');
  // Response alter
  $dfm->registerHook('response_alter', 'dfm_drupal_response_alter');
  // Delete
  $dfm->registerHook('delete_filepath_validate', 'dfm_drupal_delete_filepath_validate', $path . '/dfm.delete.inc');
  $dfm->registerHook('delete_filepath', 'dfm_drupal_delete_filepath', $path . '/dfm.delete.inc');
  // Upload
  $dfm->registerHook('upload_file_validate', 'dfm_drupal_upload_file_validate', $path . '/dfm.upload.inc');
  $dfm->registerHook('upload_file', 'dfm_drupal_upload_file', $path . '/dfm.upload.inc');
  // Rename
  $dfm->registerHook('rename_filepath', 'dfm_drupal_rename_filepath', $path . '/dfm.rename.inc');
  $dfm->registerHook('rename_dirpath', 'dfm_drupal_rename_dirpath', $path . '/dfm.rename.inc');
  // Move
  $dfm->registerHook('move_filepath_validate', 'dfm_drupal_move_filepath_validate', $path . '/dfm.move.inc');
  $dfm->registerHook('move_filepath', 'dfm_drupal_move_filepath', $path . '/dfm.move.inc');
  $dfm->registerHook('move_dirpath', 'dfm_drupal_move_dirpath', $path . '/dfm.move.inc');
  // Copy
  $dfm->registerHook('copy_filepath_validate', 'dfm_drupal_copy_filepath_validate', $path . '/dfm.copy.inc');
  $dfm->registerHook('copy_filepath', 'dfm_drupal_copy_filepath', $path . '/dfm.copy.inc');
  // Resize
  $dfm->registerHook('resize_filepath_validate', 'dfm_drupal_resize_filepath_validate', $path . '/dfm.resize.inc');
  $dfm->registerHook('resize_filepath', 'dfm_drupal_resize_filepath', $path . '/dfm.resize.inc');
  // Crop. Resize handlers will also work for crop.
  $dfm->registerHook('crop_filepath_validate', 'dfm_drupal_resize_filepath_validate', $path . '/dfm.resize.inc');
  $dfm->registerHook('crop_filepath', 'dfm_drupal_resize_filepath', $path . '/dfm.resize.inc');
  // Allow modules register custom hooks.
  foreach (variable_get('dfm_register_hook', array()) as $func => $state) {
    if ($state && function_exists($func)) {
      $func($dfm);
    }
  }
  // Allow alteration of $dfm object
  drupal_alter('dfm_register', $dfm);
  // Set a post load callback
  $dfm->registerHook('ajax_load', 'dfm_drupal_post_ajax_load');
}

/**
 * Plugin pre ajax_load callback.
 */
function dfm_drupal_pre_ajax_load($dfm) {
  // Translate strings if locale or custom strings is enabled.
  $lang = $dfm->getConf('lang');
  if (!$lang || ($lang === 'en' || !module_exists('locale')) && !variable_get('locale_custom_strings_' . $lang, FALSE)) {
    return;
  }
  // Add a flag indicating Drupal translation is on.
  $dfm->setConf('drupalI18n', TRUE);
  $jsdir = 'public://js';
  $jspath = $jsdir . '/dfm.' . $lang . '.js';
  // Translation file was created before. Just add it to page.
  if (file_exists($jspath)) {
    if ($jsurl = file_create_url($jspath)) {
      $dfm->addJs($jsurl);
    }
    return;
  }
  // Generate the list of tranlation strings.
  // We'll eventually create a translation file even if there is no translation strings.
  // Set empty strings to make sure $dfm->getConf('drupalStrings') evaluates to TRUE in if statements
  $strs = array('' => '');
  // Add strings from the template if there is no default translation file for the lang
  if (!file_exists($dfm->scriptPath('i18n/dfm.' . $lang . '.js'))) {
    if ($content = file_get_contents($dfm->scriptPath('i18n/dfm.template.js'))) {
      preg_match_all("@\n'((?:\\\\.|[^'])+)':@", $content, $matches);
      if (!empty($matches[1])) {
        foreach ($matches[1] as $str) {
          $strs[$str] = t($str);
        }
      }
    }
  }
  // Set strings as a variable so modules can add their own
  $dfm->setConf('drupalStrings', $strs);
}

/**
 * Plugin post ajax_load callback.
 */
function dfm_drupal_post_ajax_load($dfm) {
  if ($strs = $dfm->getConf('drupalStrings')) {
    $lines = array();
    foreach ($strs as $str => $t) {
      if ($str != $t) {
        $lines[$str] = "'$str':'$t'";
      }
    }
    $jsdir = 'public://js';
    if (file_prepare_directory($jsdir, FILE_CREATE_DIRECTORY)) {
      $jspath = $jsdir . '/dfm.' . $dfm->getConf('lang') . '.js';
      if (@file_put_contents($jspath, 'dfm.extend(dfm.i18n, {' . implode(',', $lines) . '});')) {
        drupal_chmod($jspath, variable_get('file_chmod_file', 0664));
        if ($jsurl = file_create_url($jspath)) {
          $dfm->addJs($jsurl);
        }
      }
    }
  }
}

/**
 * Implements plugin_hook_response_alter().
 */
function dfm_drupal_response_alter($dfm) {
  // Convert drupal messages to DFM messages
  if ($messages = drupal_get_messages()) {
    foreach ($messages as $type => $arr) {
      foreach ($arr as $message) {
        $dfm->setMessage($message, array(), $type);
      }
    }
  }
}

/**
 * Implements hook_preprocess_dfm_page().
 */
function template_preprocess_dfm_page(&$variables) {
  // Set default configuration values
  $page = &$variables['page'];
  $path = dfm_script_path();
  $page['libUrl'] = base_path() . $path;
  $page['miscDir'] = $page['libUrl'] . '/core/misc';
  $page['qs'] = $qs = '?' . variable_get('css_js_query_string', '0');
  $page['cssUrls']['core'] = $page['miscDir'] . '/dfm.css' . $qs;
  $page['jsUrls']['jquery'] = $page['miscDir'] . '/jquery.js';
  $page['jsUrls']['core'] = $page['miscDir'] . '/dfm.js' . $qs;
  if (!isset($page['title'])) $page['title'] = t('File Browser');
  if (!isset($page['scriptConf']['url'])) $page['scriptConf']['url'] = url($_GET['q']);
  if (!isset($page['head'])) $page['head'] = '';
}

/**
 * Returns a file record by path.
 */
function dfm_file_by_path($filepath, $create = FALSE) {
  if ($files = file_load_multiple(array(), array('uri' => $filepath))) {
    return reset($files);
  }
  $file = FALSE;
  if ($create) {
    $file = (object) array(
      'uid' => $GLOBALS['user']->uid,
      'filename' => dfm_basename($filepath),
      'uri' => $filepath,
      'filemime' => file_get_mimetype($filepath),
      'filesize' => filesize($filepath),
      'timestamp' => filemtime($filepath),
      'status' => 1,
      'dfm' => 1,
    );
  }
  return $file;
}

/**
 * Returns all references to a file except own references.
 */
function dfm_get_file_references($file) {
  $refs = file_usage_list($file);
  // Prevent Dfm returning ref count. Allow deletion of imce files for users who also use the popular file browser.
  unset($refs['dfm'], $refs['imce']);
  return $refs;
}

/**
 * Deletes a file record by path.
 */
function dfm_delete_filepath_record($filepath) {
  if ($file = dfm_file_by_path($filepath)) {
    module_invoke_all('file_delete', $file);
    module_invoke_all('entity_delete', $file, 'file');
    db_delete('file_managed')->condition('fid', $file->fid)->execute();
    db_delete('file_usage')->condition('fid', $file->fid)->execute();
    return TRUE;
  }
  return FALSE;
}

/**
 * Renames a file record by path.
 */
function dfm_rename_filepath_record($filepath, $newpath) {
  if ($file = dfm_file_by_path($filepath)) {
    $values = array(':filename' => dfm_basename($newpath), ':uri' => $newpath, ':fid' => $file->fid);
    db_query("UPDATE {file_managed} SET filename = :filename, uri = :uri WHERE fid = :fid", $values);
    return TRUE;
  }
  return FALSE;
}

/**
 * Validates disk quota and returns an error.
 */
function dfm_get_quota_error($file, $quota = 0, $extra = 0) {
  if ($quota) {
    $used = file_space_used($file->uid);
    if (($used + $file->filesize + $extra) > $quota) {
      return t("There isn't sufficient disk quota (%used/%quota) to save %filename (%filesize).", array('%filename' => $file->filename, '%filesize' => format_size($file->filesize), '%used' => format_size($used), '%quota' => format_size($quota)));
    }
  }
}

/**
 * Saves a dfm file record.
 * May use existing record by checking the filepath.
 * Unlike file_save, this does not change timestamp and filesize properties.
 */
function dfm_write_file_record($file, $skipcheck = FALSE) {
  // Existing record
  if (!$skipcheck && $original = dfm_file_by_path($file->uri)) {
    $file->fid = $original->fid;
    $file->original = $original;
    module_invoke_all('file_presave', $file);
    module_invoke_all('entity_presave', $file, 'file');
    if ($ret = drupal_write_record('file_managed', $file, array('fid'))) {
      module_invoke_all('file_update', $file);
      module_invoke_all('entity_update', $file, 'file');
      unset($file->original);
    }
  }
  // New record
  else {
    module_invoke_all('file_presave', $file);
    module_invoke_all('entity_presave', $file, 'file');
    if ($ret = drupal_write_record('file_managed', $file)) {
      module_invoke_all('file_insert', $file);
      module_invoke_all('entity_insert', $file, 'file');
    }
  }
  return $ret;
}

/**
 * Multi-byte safe basename.
 * Also checks for backslash on Windows.
 */
function dfm_basename($path) {
  if (DIRECTORY_SEPARATOR === '\\') {
    $path = rtrim($path, '\\');
    $pos = strrpos($path, '\\');
    if ($pos !== FALSE) $path = substr($path, $pos + 1);
  }
  $path = rtrim($path, '/');
  $pos = strrpos($path, '/');
  return $pos === FALSE ? $path : substr($path, $pos + 1);
}



/**
 * Returns url of the root file directory.
 */
function dfm_root_url($rootdir, $absolute = FALSE) {
  $url = file_create_url($rootdir);
  if (!$absolute) {
    global $base_root;
    $len = strlen($base_root);
    if (substr($url, 0, $len+1) == ($base_root . '/')) {
      $url = substr($url, $len);
    }
  }
  return rtrim($url, '/');
}

/**
 * Encodes a filepath as an url.
 */
function dfm_encode_path($path) {
  return str_replace('%2F', '/', rawurlencode($path));
}

/**
 * Returns a file manager instance for a user.
 */
function dfm_user_fm($user = NULL, $scheme = NULL) {
  if ($conf = dfm_user_conf($user, $scheme)) {
    return dfm_create_fm($conf);
  }
}

/**
 * Returns processed profile configuration for a user.
 */
function dfm_user_conf($user = NULL, $scheme = NULL) {
  if ($profile = dfm_user_profile($user, $scheme)) {
    return dfm_process_profile($profile)->conf;
  }
}

/**
 * Creates a file manager instance with a user conf.
 */
function dfm_create_fm($conf) {
  if (!isset($conf['scriptDirPath'])) {
    $conf['scriptDirPath'] = dfm_script_path();
  }
  require_once $conf['scriptDirPath'] . '/core/Dfm.php';
  return new Dfm($conf);
}

/**
 * Checks if a file uri is accessible by a user with Dfm.
 */
function dfm_check_file_uri($uri, $user = NULL) {
  list($scheme, $path) = explode('://', $uri, 2);
  if ($scheme && $path) {
    if ($fm = dfm_user_fm($user, $scheme)) {
      return $fm->checkFileUri($uri);
    }
  }
}

/**
 * Checks if the selected file paths(relative) are accessible by a user with Dfm.
 * Returns the accessible file uris.
 */
function dfm_check_file_paths($paths, $user = NULL, $scheme = NULL) {
  $ret = array();
  if ($fm = dfm_user_fm($user, $scheme)) {
    foreach ($paths as $path) {
      if ($uri = $fm->checkFile($path)) {
        $ret[$path] = $uri;
      }
    }
  }
  return $ret;
}

/**
 * Update body of nodes/blocks on file move/rename
 */
function dfm_check_fix_file_move($dfm, $filepath, $newpath, $isdir = FALSE) {
  try {
    // Update body of nodes/blocks
    if ($dfm->getConf('fixBodyOnMove')) {
      if ($url = file_create_url($filepath)) {
        if ($newurl = file_create_url($newpath)) {
          // Fix Absolute urls
          dfm_fix_file_move($url, $newurl, $isdir);
          // Fix relative urls.
          $relurl = strpos($url, $GLOBALS['base_url'] . '/') === 0 ? substr($url, strlen($GLOBALS['base_url'])) : FALSE;
          $newrelurl = strpos($newurl, $GLOBALS['base_url'] . '/') === 0 ? substr($newurl, strlen($GLOBALS['base_url'])) : FALSE;
          if ($relurl && $newrelurl) {
            dfm_fix_file_move($relurl, $newrelurl, $isdir);
          }
        }
      }
    }
  }
  catch(Exception $e) {
    watchdog_exception('dfm', $e);
  }
}

/**
 * Update moved/renamed file urls in body of nodes/blocks.
 */
function dfm_fix_file_move($url, $newurl, $isdir = FALSE) {
  $url = '"' . $url;
  $newurl = '"' . $newurl;
  if ($isdir) {
    if (substr($url, -1) !== '/') {
      $url .= '/';
    }
    if (substr($newurl, -1) !== '/') {
      $newurl .= '/';
    }
  }
  else {
    $url .= '"';
    $newurl .= '"';
  }
  // Block body
  $tokens = array(':url' => $url, ':newurl' => $newurl);
  db_query("UPDATE {block_custom} SET body = REPLACE(body, :url, :newurl)", $tokens);
  // Node body
  $result = db_select('field_data_body', 'b')->fields('b', array('entity_type', 'entity_id'))->condition('entity_type', 'node')->condition('body_value', '%' . db_like($url) . '%', 'LIKE')->execute();
  $types = array();
  foreach ($result as $row) {
    $types[$row->entity_type][] = $row->entity_id;
  }
  foreach ($types as $type => $ids) {
    $tokens[':type'] = $type;
    $tokens[':ids'] = $ids;
    db_query("UPDATE {field_data_body} SET body_value = REPLACE(body_value, :url, :newurl), body_summary = REPLACE(body_summary, :url, :newurl) WHERE entity_type = :type AND entity_id IN(:ids)", $tokens);
    // Clear field cache
    foreach ($ids as $id) {
      cache_clear_all("field:$type:$id", 'cache_field');
    }
  }
}