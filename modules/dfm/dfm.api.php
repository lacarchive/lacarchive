<?php

/**
 * @file
 * Hooks provided by the DFM module.
 */

/**
 * @addtogroup hooks
 * @{
 */

/**
 * Add/alter permission names.
 * Use this hook if your module defines a new file/folder operation.
 *
 * @param $perms
 *   An array of file and folder permissions.
 */
function hook_dfm_perm_info_alter(&$perms) {
  // Define permission of custom x operation.
  $perms['file']['x'] = t('X files');
  $perms['folder']['x'] = t('X folders');
}

/**
 * Alter DFM configuration of a user.
 *
 * @param $conf
 *   An array of configuration parameters including enabled folders.
 * @param $user
 *   Drupal user(usually the logged in user) who will use the configuration.
 */
function hook_dfm_conf_alter(&$conf, $user) {
  // Add <root>/foo folder with all available permissions.
  $conf['dirConf']['foo'] = array(
    'perms' => array(
      'all' => 1,
    ),
    'subdirConf' => array(
      'inherit' => 1,
    ),
  );
  // Add <root>/user[User-ID] folder with limited permissions and no subfolder inheritance.
  $conf['dirConf']['user' . $user->uid] = array(
    'perms' => array(
      'listFiles' => 1,
      'uploadFiles' => 1,
      'deleteFiles' => 1,
    ),
  );
  // Override upload extensions
  $conf['uploadExtensions'] = array('txt', 'zip');
}

/**
 * Register hooks to run on ajax operations.
 *
 * @param $dfm
 *   File manager instance object.
 */
function hook_dfm_register_alter($dfm) {
  // Set a load callback which can be used to add js/css files by $dfm->addJs/addCss(URL)
  $dfm->registerHook('ajax_load', 'my_dfm_ajax_load');
  // Set callback for custom x operation in an inc file.
  $dfm->registerHook('ajax_x', 'my_dfm_ajax_x', drupal_get_path('module', 'my') . '/my.dfm.inc');
  // Register delete validation callback
  $dfm->registerHook('delete_filepath_validate', 'my_dfm_delete_filepath_validate');
  // Possible hooks:
  // upload_file_validate, upload_file, ajax_upload
  // delete_filepath_validate, delete_filepath, delete_dirpath_validate, delete_dirpath, ajax_delete
  // rename_filepath_validate, rename_filepath, rename_dirpath_validate, rename_dirpath, ajax_rename
  // move_filepath_validate, move_filepath, move_dirpath_validate, move_dirpath, ajax_move
  // copy_filepath_validate, copy_filepath, copy_dirpath_validate, copy_dirpath, ajax_copy
  // resize_filepath_validate, resize_filepath, ajax_resize
  // crop_filepath_validate, crop_filepath, ajax_crop
  // response_alter, output_alter, exit
}

/**
 * Add/alter supported widget names for file field integration.
 *
 * @param $widgets
 *   An array of field widget names.
 */
function hook_dfm_filefield_supported_widgets_alter(&$widgets) {
  $widgets[] = 'my_file_widget';
}

/**
 * Preprocess DFM page.
 *
 * @param $variables
 *   An array of theme variables.
 */
function hook_preprocess_dfm_page(&$variables) {
  $page = &$variables['page'];
  // Define always included js/css
  $path = base_path() . drupal_get_path('module', 'my');
  $query_string = $page['qs'];
  $page['jsUrls']['my_js'] = $path . '/my.dfm.js' . $query_string;
  $page['cssUrls']['my_css'] = $path . '/my.dfm.css' . $query_string;
  // Change title
  $page['title'] = t('My File Browser');
  // Add head elements
  $page['head'] .= '<link rel="shortcut icon" href="/misc/favicon.ico" type="image/vnd.microsoft.icon" />';
}

/**
 * @} End of "addtogroup hooks".
 */
