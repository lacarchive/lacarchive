<?php

/**
 * Provide a helper to check if file paths are ignored.
 */
trait IgnoredPathsTrait {

  /**
   * Check if the file path is ignored.
   *
   * @param string $file_path
   *   The file path.
   *
   * @return bool
   *   TRUE if file path is ignored, else FALSE.
   */
  protected static function isIgnoredPath($file_path) {
    $paths = variable_get('automatic_updates_ignored_paths', "sites/all/modules/*\nsites/all/themes/*");
    if (drupal_match_path($file_path, $paths)) {
      return TRUE;
    }
    return FALSE;
  }

}
