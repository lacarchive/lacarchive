<?php

/**
 * Provide a helper to get project info.
 */
trait ProjectInfoTrait {

  /**
   * Returns an array of info files information of available extensions.
   *
   * @return array
   *   An associative array of extension information arrays, keyed by extension
   *   name.
   */
  protected static function getInfos() {
    $infos = [];
    // Find extensions.
    $extensions = drupal_system_listing('/^' . DRUPAL_PHP_FUNCTION_PATTERN . '\.info$/', 'modules', $key = 'name', $min_depth = 1);
    $extensions = array_merge($extensions, drupal_system_listing('/^' . DRUPAL_PHP_FUNCTION_PATTERN . '\.info$/', 'themes', $key = 'name', $min_depth = 1));
    foreach ($extensions as $extension) {
      if (file_exists($info_file = dirname($extension->uri) . '/' . $extension->name . '.info')) {
        // Get the .info file for the module or theme this file belongs to.
        $infos[$extension->name] = drupal_parse_info_file($info_file);
        $info = &$infos[$extension->name];
        $info['packaged'] = isset($info['project']) ? $info['project'] : FALSE;
        $info['install path'] = dirname($extension->uri);
        $info['project'] = self::getProjectName($extension->name, $info);
        $info['version'] = self::getExtensionVersion($info);
      }
    }
    $system = isset($infos['system']) ? $infos['system'] : NULL;
    $infos = array_filter($infos, static function (array $info, $project_name) {
      return $info && $info['project'] === $project_name;
    }, ARRAY_FILTER_USE_BOTH);
    if ($system) {
      $infos['drupal'] = $system;
    }

    return $infos;
  }

  /**
   * Get the extension's project name.
   *
   * @param string $extension_name
   *   The extension name.
   * @param array $info
   *   The extension's info.
   *
   * @return string
   *   The project name or fallback to extension name if project is undefined.
   */
  protected static function getProjectName($extension_name, array $info) {
    $project_name = $extension_name;
    if (isset($info['project'])) {
      $project_name = $info['project'];
    }
    if (strpos($info['install path'], 'modules') === 0) {
      $project_name = 'drupal';
    }
    if (strpos($info['install path'], 'themes') === 0) {
      $project_name = 'drupal';
    }
    return $project_name;
  }

  /**
   * Get the extension version.
   *
   * @param array $info
   *   The extension's info.
   *
   * @return string|null
   *   The version or NULL if undefined.
   */
  protected static function getExtensionVersion(array $info) {
    $extension_name = $info['project'];
    if (isset($info['version']) && strpos($info['version'], '-dev') === FALSE) {
      return $info['version'];
    }
    watchdog('automatic_updates', 'Version cannot be located for @extension', ['@extension' => $extension_name], WATCHDOG_ERROR);
    return NULL;
  }

}
