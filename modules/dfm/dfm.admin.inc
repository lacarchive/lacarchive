<?php

/**
 * @file
 * Serves administration pages of Drupella FM.
 */

/**
 * Administaration page.
 */
function dfm_admin() {
  $path = 'admin/config/media/dfm/profile';
  $header = array(t('Profile name'), t('Operations'));
  $rows = array();
  $sw = dfm_swrps(TRUE);
  foreach (dfm_all_profiles() as $pid => $profile) {
    $ppath = $path . '/' . $pid;
    $ops = array(l(t('Edit'), $ppath), $pid == 1 ? t('Delete') : l(t('Delete'), $ppath . '/delete'));
    foreach ($sw as $scheme => $info) {
      $ops[] = l(t('Test with @file-system', array('@file-system' => $info['name'])), $ppath . '/demo/' . $scheme, array('attributes' => array('class' => array('dfm-demo-link'))));
    }
    $rows[] = array(check_plain($profile->name), implode(' | ', $ops));
  }
  $rows[] = array(l(t('Add new profile'), $path), '');
  $output['title'] = array('#markup' => '<h2 class="title">' . t('Configuration profiles') . '</h2>');
  $output['table'] = array('#theme' => 'table', '#header' => $header, '#rows' => $rows);
  $output['form'] = drupal_get_form('dfm_admin_form');
  $output['#attached']['js'] = array(drupal_get_path('module', 'dfm') . '/dfm.admin.js');
  // Set warning about anonymous user access
  if (empty($_POST)) {
    $roles = variable_get('dfm_roles_profiles', array());
    $rid = DRUPAL_ANONYMOUS_RID;
    if (!empty($roles[$rid]['public_pid']) || !empty($roles[$rid]['private_pid'])) {
      drupal_set_message(t('Your configuration allows anonymous users access to Drupella FM. Please make sure this is not a misconfiguration.'), 'warning');
    }
  }
  return $output;
}

/**
 * Administration form.
 */
function dfm_admin_form($form, &$form_state) {
  // Role-profile assignments.
  $form['roles'] = array('#tree' => TRUE);
  $roles = dfm_sorted_roles_profiles();
  $poptions = dfm_profile_options();
  $schemes = array_keys(dfm_swrps(TRUE));
  foreach ($roles as $rid => $role) {
    $form['roles'][$rid]['name'] = array(
      '#markup' => check_plain($role['name']),
    );
    foreach ($schemes as $scheme) {
      $key = $scheme . '_pid';
      $form['roles'][$rid][$key] = array(
        '#type' => 'select',
        '#options' => $poptions,
        '#default_value' => $role[$key],
      );
    }
    $form['roles'][$rid]['weight'] = array(
      '#type' => 'textfield',
      '#size' => 3,
      '#default_value' => $role['weight'],
      '#attributes' => array('class' => array('rp-weight')),
    );
  }
  // General settings
  $form['settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Common settings'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  );
  // Merge folders
  $form['settings']['merge_folders'] = array(
    '#type' => 'checkbox',
    '#title' => t('Merge folders from multiple profiles'),
    '#description' => t('A user with multiple roles gets only the first matching profile in the list. This option adds folders to the assigned profile from other matching profiles as well.'),
    '#default_value' => variable_get('dfm_merge_folders', 0),
  );
  // Integrated textareas
  $form['settings']['textareas'] = array(
    '#type' => 'textfield',
    '#title' => t('Enable image/link insertion into textareas'),
    '#default_value' => variable_get('dfm_textareas', ''),
    '#maxlength' => NULL,
    '#description' => t('Enter comma separated textarea IDs under which you want to enable a widget that allows to select files from Drupella FM and inserts the HTML code into the textarea. This widget works for only plain textareas without a WYSIWYG editor. ID of Body fields in most node types starts with <strong>edit-body*</strong>. The * character can be used as a wildcard.'),
  );
  // Absolute URLs
  $form['settings']['abs_urls'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable absolute URLs'),
    '#description' => t('Make the file manager return absolute file URLs to other applications.'),
    '#default_value' => variable_get('dfm_abs_urls', 0),
  );
  $form['submit'] = array('#type' => 'submit', '#value' => t('Save configuration'));
  $form['#theme'] = 'dfm_admin_form';
  return $form;
}

/**
 * Admin form theme.
 */
function dfm_admin_form_theme($variables) {
  $form = $variables['form'];
  $sw = dfm_swrps(TRUE);
  $dsw = reset($sw);
  // Prepare table
  $header = array(t('User role'));
  $rows = array(array(t('Site owner')));
  $keys = array('name');
  $poptions = dfm_profile_options();
  foreach ($sw as $scheme => $info) {
    $header[] = $info['name'];
    $keys[] = $scheme . '_pid';
    $rows[0][] = check_plain($poptions[1]);
  }
  $rows[0][] = '';
  $header[] = t('Weight');
  $keys[] = 'weight';
  foreach (element_children($form['roles']) as $rid) {
    $cells = array();
    foreach ($keys as $key) {
      $cells[] = drupal_render($form['roles'][$rid][$key]);
    }
    $rows[] = array('data' => $cells, 'class' => array('draggable'));
  }
  $output = '<h2 class="title">'. t('Role-profile assignments') .'</h2>';
  $output .= theme('table', array('header' => $header, 'rows' => $rows, 'attributes' => array('id' => 'dfm-roles-profiles')));
  $output .= '<div class="form-item"><div class="description">'. t('Assign profiles to user roles for available file systems. Your default file system is %name. Users with multiple roles get the first profile assigned.', array('%name' => $dsw['name'])) .'</div></div>';
  $output .= drupal_render_children($form);
  drupal_add_tabledrag('dfm-roles-profiles', 'order', 'sibling', 'rp-weight');
  return $output;
}

/**
 * Submits admin form.
 */
function dfm_admin_form_submit($form, &$form_state) {
  $roles = $form_state['values']['roles'];
  uasort($roles, 'dfm_weight_sort');
  variable_set('dfm_roles_profiles', $roles);
  foreach ($form_state['values']['settings'] as $key => $value) {
    variable_set('dfm_' . $key, $value);
  }
  drupal_set_message(t('Changes have been saved.'));
}

/**
 * Add-Edit-Delete profiles.
 */
function dfm_profile_operations($pid = NULL, $op = NULL, $a = NULL) {
  // Add
  if (!isset($pid)) {
    return drupal_get_form('dfm_profile_form');
  }
  if ($profile = dfm_load_profile($pid)) {
    // Edit
    if (!isset($op)) {
      return drupal_get_form('dfm_profile_form', $profile);
    }
    // Delete
    if ($op === 'delete' && $pid != 1) {
      return drupal_get_form('dfm_profile_delete_form', $profile);
    }
    // Demo
    if ($op === 'demo') {
      $scheme = isset($a) ? $a : file_default_scheme();
      $sw = dfm_swrps();
      if (isset($sw[$scheme])) {
        $profile->scheme = $scheme;
        return dfm_profile_demo($profile);
      }
    }
  }
  drupal_access_denied();
}

/**
 * Profile form.
 */
function dfm_profile_form($form, &$form_state, $profile = NULL) {

  if ($profile) {
    drupal_set_title(check_plain($profile->name));
  }
  else {
    dfm_inc('install');
    $profile = (object) array(
      'pid' => 0,
      'name' => t('New profile'),
      'conf' => dfm_default_member_profile_conf()
    );
  }

  // Profile
  $pform['pid'] = array(
    '#type' => 'value',
    '#value' => $profile->pid
  );
  $pform['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Profile name'),
    '#default_value' => $profile->name,
    '#required' => TRUE,
    '#weight' => -1,
  );

  // Profile configuration options
  $conf = $profile->conf;
  $cform['uploadMaxSize'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum upload size'),
    '#default_value' => dfm_arr_val($conf, 'uploadMaxSize', 2),
    '#description' => t('Set to 0 for no limit.') .' '. t('Your PHP settings limit the maximum file size per upload to %size.', array('%size' => format_size(file_upload_max_size()))),
    '#field_suffix' => t('MB'),
  );
  $cform['uploadQuota'] = array(
    '#type' => 'textfield',
    '#title' => t('Disk quota'),
    '#default_value' => dfm_arr_val($conf, 'uploadQuota', 20),
    '#description' => t('Maximum disk space that can be allocated by user files. Files uploaded by other means than Drupal are not counted.') . ' ' . t('Set to 0 for no limit.'),
    '#field_suffix' => t('MB'),
  );
  $cform['uploadExtensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Permitted file extensions'),
    '#default_value' => dfm_arr_val($conf, 'uploadExtensions', 'jpg png gif'),
    '#maxlength' => 255,
    '#description' => t('Specify file extensions that can be uploaded. Separate extensions with a space and do not include the leading dot.'),
    '#required' => TRUE,
  );
  $cform['imgMaxDim'] = array(
    '#type' => 'textfield',
    '#title' => t('Maximum image dimensions'),
    '#default_value' => dfm_arr_val($conf, 'imgMaxDim', 0),
    '#description' => t('Images exceeding this value will be scaled down to fit.') . ' ' . t('Set to 0 for no limit.'),
    '#field_suffix' => '<kbd>' . t('WIDTHxHEIGHT') . '</kbd>',
  );
  // Search settings
  $cform['search'] = array(
    '#type' => 'fieldset',
    '#title' => t('Search settings'),
    '#collapsible' => TRUE,
    '#collapsed' => !dfm_arr_val($conf, 'searchOn', 0),
    // Set profile conf as its parent. Descendent items will inherit this.
    '#parents' => array('profile', 'conf'),
  );
  $cform['search']['searchOn'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable file searching'),
    '#default_value' => dfm_arr_val($conf, 'searchOn', 0),
  );
  $cform['search']['searchLimit'] = array(
    '#type' => 'textfield',
    '#title' => t('Search limit'),
    '#default_value' => dfm_arr_val($conf, 'searchLimit', 0),
    '#description' => t('For performance reasons you may want to limit the number of results returned from the server to the client. Searching files on server side will stop when the limit is reached. This will not affect client side searching that is performed on cached items.') . ' ' . t('Set to 0 for no limit.'),
    '#states' => array(
      'visible' => array(
        '#edit-profile-conf-searchon' => array('checked' => TRUE),
      ),
    ),
  );

  // Custom plugins
  if ($plugins = dfm_extension_list('plugins')) {
    $cform['plugins'] = array(
      '#type' => 'checkboxes',
      '#title' => t('Enabled plugins'),
      '#options' => array_combine($plugins, $plugins),
      '#default_value' => dfm_arr_val($conf, 'plugins', array()),
      '#weight' => 1,
      '#description' => t('Enable plugins to add new functionality to the file manager.'),
    );
  }
  // Custom themes
  if ($themes = dfm_extension_list('themes')) {
    $cform['theme'] = array(
      '#type' => 'radios',
      '#title' => t('Active theme'),
      '#options' => array('' => t('Default')) + array_combine($themes, $themes),
      '#default_value' => dfm_arr_val($conf, 'theme', ''),
      '#weight' => 1,
      '#description' => t('Select a theme to change the appearance of the file manager.'),
    );
  }

  // Advanced settings
  $cform['advanced'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced configuration'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    // Set profile conf as its parent. Descendent items will inherit this.
    '#parents' => array('profile', 'conf'),
    '#weight' => 2,
  );
  // Image thumbnails
  if (function_exists('image_styles')) {
    $options = array('' => t('none'));
    foreach (image_styles() as $name => $style) {
      $options[$name] = $name;
    }
    $cform['advanced']['thumbStyle'] = array(
      '#type' => 'select',
      '#title' => t('Show image icons as thumbnails'),
      '#options' => $options,
      '#description' => t('Select a thumbnail style from the list to make the file browser display icons of image files as thumbnails(32x32).'),
      '#default_value' => dfm_arr_val($conf, 'thumbStyle', ''),
    );
  }
  // Chroot jail
  $cform['advanced']['chrootJail'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable chroot jail'),
    '#description' => t("This option sets the user's top-most folder as the root of folder tree. So, instead of the tree structure home>users>user42 there will be only home folder pointing to users/user42. Note that, this won't work for multiple top-most folders generating different branches."),
    '#default_value' => dfm_arr_val($conf, 'chrootJail', 0),
  );
  // Ignore file usage
  $cform['advanced']['ignoreFileUsage'] = array(
    '#type' => 'checkbox',
    '#title' => t('Ignore file usage'),
    '#description' => t('By default the file manager avoids deletion or overwriting of files that are in use by other Drupal modules. Enabling this option skips the file usage check. Not recommended!'),
    '#default_value' => dfm_arr_val($conf, 'ignoreFileUsage', 0),
  );
  // Image copy
  $cform['advanced']['imgCopy'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable image copy'),
    '#description' => t('Allow image operations to be performed optionally on a copy instead of the original image regardless of the file copy permission. A new option will appear in image widgets allowing to work on a copy.'),
    '#default_value' => dfm_arr_val($conf, 'imgCopy', 0),
  );
  // Image upscale
  $cform['advanced']['imgUpscale'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable image upscaling'),
    '#description' => t('Allow image dimensions to be set to higher values than the original ones during resizing. Maximum allowed dimensions restriction is still applied.'),
    '#default_value' => dfm_arr_val($conf, 'imgUpscale', 0),
  );
  // Disable auto-scale on upload
  $cform['advanced']['uploadNoScale'] = array(
    '#type' => 'checkbox',
    '#title' => t('Disable auto-scale'),
    '#description' => t('Disable scaling images down to the maximum allowed dimensions during upload. The ones bigger than the allowed dimensions will be rejected.'),
    '#default_value' => dfm_arr_val($conf, 'uploadNoScale', 0),
  );
  // Image extensions
  $cform['advanced']['imgExtensions'] = array(
    '#type' => 'textfield',
    '#title' => t('Image extensions'),
    '#default_value' => dfm_arr_val($conf, 'imgExtensions', 'jpg jpeg png gif'),
    '#maxlength' => 255,
    '#description' => t('You can disable image handling by leaving this field empty. It can drastically increase browsing performance for remote(S3, etc.) directories with lots of images.'),
  );
  // Fix body field on move
  $cform['advanced']['fixBodyOnMove'] = array(
    '#type' => 'checkbox',
    '#title' => t('Fix urls in body fields on file move'),
    '#description' => t('When files/folders are moved or renamed hard-coded URLs in body fields are not updated. Check this box if you want DFM try fixing them. Note that this requires some expensive database queries which may slow down move and rename operations.'),
    '#default_value' => dfm_arr_val($conf, 'fixBodyOnMove', 0),
  );

  // Directory configurations
  $cform['dirConfRaw']['#theme'] = 'dfm_directory_form';
  $cform['dirConfRaw']['#weight'] = 3;
  $parents = array('profile', 'conf', 'dirConfRaw');
  $i = 0;
  if (!empty($conf['dirConfRaw'])) {
    foreach ($conf['dirConfRaw'] as $folder) {
      $cform['dirConfRaw'][$i] = dfm_directory_form(array_merge($parents, array($i)), $folder);
      $i++;
    }
  }
  $cform['dirConfRaw'][$i] = dfm_directory_form(array_merge($parents, array($i)));
  $cform['dirConfRaw'][$i + 1] = dfm_directory_form(array_merge($parents, array($i+1)));

  $form = array('#tree' => TRUE);
  $form['profile'] = $pform;
  $form['profile']['conf'] = $cform;
  $form['submit'] = array('#type' => 'submit', '#value' => t('Save configuration'));
  $form['#attributes']['class'][] = 'dfm-profile-form';
  $form['#attached']['css'][] = drupal_get_path('module', 'dfm') . '/dfm.admin.css';
  return $form;
}

/**
 * Directory settings form.
 */
function dfm_directory_form($parents, array $dirconf = array()) {
  // Backwards compatibility
  if (is_int($parents)) {
    $parents = array('profile', 'conf', 'dirConfRaw', $parents);
  }
  $form = array(
    '#type' => 'container',
    '#attributes' => array('class' => array('folder-container')),
    '#parents' => $parents,
  );
  // Directory name
  $form['dirname'] = array(
    '#type' => 'textfield',
    '#default_value' => dfm_arr_val($dirconf, 'dirname', ''),
    '#field_prefix' => '&lt;' . t('root') . '&gt;' . '/',
  );
  // Configuration elements
  $form += dfm_directory_form_elements($parents, $dirconf);
  // Sub-directory configuration
  $subdirconf = dfm_arr_val($dirconf, 'subdirConf', array());
  $parents[] = 'subdirConf';
  $form['subdirConf'] = dfm_directory_form_elements($parents, $subdirconf, TRUE);
  return $form;
}

/**
 * Theme callback for directory form.
 */
function dfm_directory_form_theme($variables) {
  $form = $variables['form'];
  $output = '<h2>' . t('Folders') . '</h2>';
  if (module_exists('token')) {
    $fieldset = array(
      '#theme' => 'fieldset',
      '#title' => t('Available tokens for folder names'),
      '#attributes' => array('class' => array('collapsible', 'collapsed')),
      '#children' => theme('token_tree', array('token_types' => array('user', 'site', 'date'), 'global_types' => FALSE , 'click_insert' => FALSE)),
    );
    $output .= drupal_render($fieldset);
  }
  else {
    $output .= '<h4 class="description token">' . t('Available tokens for folder names') . ': [user:name], [user:uid]</h4>';
  }
  $output .= drupal_render_children($form);
  return $output;
}

/**
 * Profile form validate.
 */
function dfm_profile_form_validate($form, &$form_state) {
  $profile = &$form_state['values']['profile'];
  $conf = &$profile['conf'];
  // Check image max dimensions
  if ($conf['imgMaxDim'] && !preg_match('/^\d+x\d+$/', $conf['imgMaxDim'])) {
    return form_set_error('profile][conf][imgMaxDim',  t('Dimensions must be specified in <kbd>WIDTHxHEIGHT</kbd> format.'));
  }
  // Check directory configurations
  $dirs = array();
  $err_field = 'profile][conf][dirConfRaw]';
  foreach ($conf['dirConfRaw'] as $i => $dirconf) {
    $dirname = $dirconf['dirname'];
    // Skip empty paths
    if ($dirname == '') {
      continue;
    }
    // Check duplicates
    if (isset($dirs[$dirname])) {
      return form_set_error($err_field . '[' . $i . '][dirname',  t('Duplicate paths are not allowed.'));
    }
    // Check if it is proper relative path
    if (!dfm_check_dirname($dirname)) {
      return form_set_error($err_field . '[' . $i . '][dirname',  t('Invalid folder path.'));
    }
    // Simplify conf structure. From deep to shalllow
    // Sub-sub inheritance
    if (empty($dirconf['subdirConf']['subdirConf']['inherit'])) {
      unset($dirconf['subdirConf']['subdirConf']);
    }
    // Sub permissions
    if (!$dirconf['subdirConf']['perms'] = array_filter($dirconf['subdirConf']['perms'])) {
      unset($dirconf['subdirConf']['perms']);
    }
    // Sub conf
    if (!$dirconf['subdirConf'] = array_filter($dirconf['subdirConf'])) {
      unset($dirconf['subdirConf']);
    }
    // Sub conf inheritance. No need for perms if inheritance is checked.
    else if (!empty($dirconf['subdirConf']['inherit'])) {
      unset($dirconf['subdirConf']['perms'], $dirconf['subdirConf']['subdirConf']);
    }
    // Permissions
    if (!$dirconf['perms'] = array_filter($dirconf['perms'])) {
      unset($dirconf['perms']);
      // No need for inherited subconf if there is no perm to inherit
      if (!empty($dirconf['subdirConf']['inherit'])) {
        unset($dirconf['subdirConf']);
      }
    }
    // Save
    $dirs[$dirname] = $dirconf;
  }
  // Require at least one directory defined.
  if (empty($dirs)) {
    return form_set_error($err_field . '[0][dirname',  t('You must define at least one folder.'));
  }
  $conf['dirConfRaw'] = array_values($dirs);
  // Check themes and plugins. Unset if empty.
  if (isset($conf['plugins']) && !($conf['plugins'] = array_keys(array_filter($conf['plugins'])))) {
    unset($conf['plugins']);
  }
  if (isset($conf['theme']) && $conf['theme'] == '') {
    unset($conf['theme']);
  }
  // Unset empty variables that normally default to false.
  foreach (array('thumbStyle', 'chrootJail', 'ignoreFileUsage', 'imgCopy', 'imgUpscale', 'uploadNoScale', 'fixBodyOnMove') as $key) {
    if (isset($conf[$key]) && !$conf[$key]) {
      unset($conf[$key]);
    }
  }
  // Unset default image extensions
  if (isset($conf['imgExtensions']) && $conf['imgExtensions'] === 'jpg jpeg png gif') {
    unset($conf['imgExtensions']);
  }
}

/**
 * Profile form submit.
 */
function dfm_profile_form_submit($form, &$form_state) {
  $profile = (object) $form_state['values']['profile'];
  $profile->conf = serialize($profile->conf);
  $message = $profile->pid ? t('The changes have been saved.') : t('Profile has been added.');
  if (drupal_write_record('dfm_profiles', $profile, $profile->pid ? array('pid') : array())) {
    drupal_set_message($message);
    $form_state['redirect'] = 'admin/config/media/dfm';
  }
}

/**
 * Profile delete form
 */
function dfm_profile_delete_form($form, &$form_state, $profile) {
  $form['pid'] = array('#type' => 'value', '#value' => $profile->pid);
  return confirm_form($form,
    t('Are you sure you want to delete the profile %name?',
    array('%name' => $profile->name)),
    'admin/config/media/dfm',
    '',
    t('Delete'),
    t('Cancel')
  );
}

/**
 * Profile delete form submit
 */
function dfm_profile_delete_form_submit($form, &$form_state) {
  $pid = $form_state['values']['pid'];
  db_delete('dfm_profiles')->condition('pid', $pid)->execute();
  drupal_set_message(t('Profile has been deleted.'));
  // Update role-profile assignments.
  $rp = variable_get('dfm_roles_profiles', array());
  foreach ($rp as $rid => &$conf) {
    foreach ($conf as $key => &$val) {
      if (substr($key, -3) == 'pid') {
        if ($pid == $val) {
          $val = 0;
        }
      }
    }
  }
  variable_set('dfm_roles_profiles', $rp);
  $form_state['redirect'] = 'admin/config/media/dfm';
}

/**
 * Profile demo page.
 */
function dfm_profile_demo($profile) {
  dfm_inc('page');
  // Run the file manager
  if (!empty($_REQUEST['ajaxOp'])) {
    $conf = dfm_process_profile($profile)->conf;
    return dfm_create_fm($conf)->run();
  }
  // Install file manager
  $page['title'] = t('Profile @name demo', array('@name' => $profile->name));
  $page['scriptConf']['url'] = url($_GET['q']);
  return dfm_page($page);
}

/**
 * Profile options.
 */
function dfm_profile_options() {
  $options = array(0 => t('none'));
  foreach (dfm_all_profiles() as $pid => $profile) {
    $options[$pid] = $profile->name;
  }
  return $options;
}

/**
 * Returns the list of custom plugins.
 */
function dfm_extension_list($type) {
  $dir = dfm_script_path($type);
  $list = array();
  if ($paths = glob("$dir/*/register.php")) {
    $start = strlen($dir) + 1;
    foreach ($paths as $path) {
      $list[] = substr($path, $start, -13);
    }
  }
  return $list;
}

/**
 * Sort roles according to their weights.
 */
function dfm_sorted_roles_profiles() {
  static $sorted;
  if (!isset($sorted)) {
    $sorted = array();
    $roles = user_roles();
    $rp = variable_get('dfm_roles_profiles', array());
    $schemes = array_keys(dfm_swrps(TRUE));
    if (!isset($rp[DRUPAL_AUTHENTICATED_RID])) {
      $rp[DRUPAL_AUTHENTICATED_RID]['weight'] = 9;
      $rp[DRUPAL_ANONYMOUS_RID]['weight'] = 10;
    }
    foreach ($roles as $rid => $rname) {
      $sorted[$rid] = array(
        'name' => $rname,
        'weight' => dfm_arr_val2($rp, $rid, 'weight', 0),
      );
      foreach ($schemes as $scheme) {
        $key = $scheme . '_pid';
        $sorted[$rid][$key] = dfm_arr_val2($rp, $rid, $key, 0);
      }
    }
    uasort($sorted, 'dfm_weight_sort');
  }
  return $sorted;
}

/**
 * Weight sorting function for role profiles.
 */
function dfm_weight_sort($r1, $r2) {
  return $r1['weight'] - $r2['weight'];
}

/**
 * Returns all configuration profiles.
 */
function dfm_all_profiles($reset = FALSE) {
  static $profiles;
  if (!isset($profiles) || $reset) {
    $profiles = array();
    $result = db_query('SELECT * FROM {dfm_profiles} ORDER BY name');
    foreach ($result as $profile) {
      $profiles[$profile->pid] = $profile;
    }
  }
  return $profiles;
}

/**
 * Returns the value of an array by key.
 */
function dfm_arr_val($arr, $key, $default = NULL) {
  return isset($arr[$key]) ? $arr[$key] : $default;
}

/**
 * Returns the value of an array by a subkey.
 */
function dfm_arr_val2($arr, $key, $subkey, $default = NULL) {
  return isset($arr[$key][$subkey]) ? $arr[$key][$subkey] : $default;
}

/**
 * Checks relative directory path.
 */
function dfm_check_dirname($dirname) {
  return is_int($dirname) || is_string($dirname) && ($dirname == '.' || $dirname != '' && !preg_match('@^[\s/\.]|[\s/\.]$|\.\.|//|\\\\|/\./@', $dirname));
}

/**
 * Returns folder permission definitions.
 */
function dfm_perm_info() {
  $info = &drupal_static(__FUNCTION__);
  if (!isset($info)) {
    $info = array(
      'file' => array(
        'listFiles' => t('List files'),
        'uploadFiles' => t('Upload files'),
        'deleteFiles' => t('Delete files'),
        'renameFiles' => t('Rename files'),
        'moveFiles' => t('Move files'),
        'copyFiles' => t('Copy files'),
        'downloadFiles' => t('Download files'),
        'resize' => t('Resize images'),
        'crop' => t('Crop images'),
      ),
      'folder' => array(
        'listFolders' => t('List folders'),
        'createFolders' => t('Create folders'),
        'deleteFolders' => t('Delete folders'),
        'renameFolders' => t('Rename folders'),
        'moveFolders' => t('Move folders'),
        'copyFolders' => t('Copy folders'),
        'downloadFolders' => t('Download folders'),
      ),
    );
    drupal_alter('dfm_perm_info', $info);
  }
  return $info;
}

/**
 * Returns permissions form elements.
 */
function dfm_perm_form(array $parents, array $values = array()) {
  $perm_info = dfm_perm_info();
  $labels = array('file' => t('File permissions'), 'folder' => t('Folder permissions'));
  $name_prefix = dfm_parents_form_name($parents);
  $form['#parents'] = $parents;
  $form['all'] = array(
    '#type' => 'checkbox',
    '#title' => t('All permissions'),
    '#default_value' => !empty($values['all']),
  );
  foreach ($perm_info as $type => $perms) {
    $form[$type] = array(
      '#type' => 'fieldset',
      '#title' => $labels[$type],
      '#attributes' => array('class' => array('permission-group pg-' . $type)),
      '#parents' => $parents,
    );
    $form[$type]['#states']['invisible']['input[name="' . $name_prefix . '[all]"]']['checked'] = TRUE;
    foreach ($perms as $perm => $title) {
      $form[$type][$perm] = array(
        '#type' => 'checkbox',
        '#title' => $title,
        '#default_value' => isset($values[$perm]) ? $values[$perm] : 0,
      );
      $form[$type][$perm]['#states']['disabled']['input[name="' . $name_prefix . '[all]"]']['checked'] = TRUE;
    }
  }
  return $form;
}

/**
 * Returns form elements for a (sub)directory configuration.
 */
function dfm_directory_form_elements(array $parents, array $dirconf = array(), $is_sub = FALSE) {
  // Permissions
  $form['perms'] = array(
    '#type' => 'fieldset',
    '#title' => t('Permissions'),
    '#collapsible' => TRUE,
    '#collapsed' => !array_filter(dfm_arr_val($dirconf, 'perms', array())),
    '#attributes' => array('class' => array('folder-permissions')),
  );
  $form['perms'] += dfm_perm_form(array_merge($parents, array('perms')), dfm_arr_val($dirconf, 'perms', array()));
  // Subdir inheritance. Place under perms and set the correct parents.
  $form['perms']['inherit'] = array(
    '#type' => 'checkbox',
    '#title' => t('Apply the same file/folder permissions to all subfolders recursively'),
    '#default_value' => dfm_arr_val2($dirconf, 'subdirConf', 'inherit', 0),
    '#parents' => array_merge($parents, array('subdirConf', 'inherit')),
  );
  // Subfolder
  if ($is_sub) {
    $form['perms']['#attributes']['class'][] = 'subfolder-permissions';
    $form['perms']['#title'] = t('Subfolder permissions');
    $form['perms']['#states']['invisible']['input[name="' . dfm_parents_form_name($parents) . '[inherit]"]']['checked'] = TRUE;
  }
  return $form;
}

/**
 * Returns name prefix derived from form parents.
 */
function dfm_parents_form_name(array $parents) {
    $name = array_shift($parents);
    if ($parents) {
      $name .= '[' . implode('][', $parents) . ']';
    }
    return $name;
}