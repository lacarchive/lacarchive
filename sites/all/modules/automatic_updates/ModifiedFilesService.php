<?php

use Drupal\Signify\ChecksumList;
use Drupal\Signify\FailedCheckumFilter;
use Drupal\Signify\Verifier;

/**
 * Modified files service.
 */
class ModifiedFilesService {

  /**
   * Get list of modified files.
   *
   * @param array $extensions
   *   The list of extensions, keyed by extension name with values an info
   *   array.
   *
   * @return \Iterator
   *   The modified files.
   */
  public static function getModifiedFiles(array $extensions = []) {
    $project_root = drupal_get_path('module', 'automatic_updates');
    require_once $project_root . DIRECTORY_SEPARATOR . 'vendor' . DIRECTORY_SEPARATOR . 'autoload.php';
    $modified_files = new \ArrayIterator();
    foreach (static::getHashRequests($extensions) as $hash_info) {
      $url = reset($hash_info);
      $response = drupal_http_request($url);
      $extension_name = key($hash_info);
      if (isset($response->code) && ($response->code === '200')) {
        static::processHashes($response->data, $extensions[$extension_name], $modified_files);
      }
      else {
        // Log all the exceptions, even modules that aren't the main project.
        watchdog('automatic_updates', 'Request for @url resulted in @error.', [
          '@url' => $url,
          '@error' => $response->error,
        ], WATCHDOG_WARNING);
        // HTTP 404 is expected for modules that aren't the main project. But
        // other error codes should complain loudly.
        if ($response->code !== '404') {
          throw new \RuntimeException(sprintf('Request for %s resulted in %s.', $url, $response->error));
        }
      }
    }

    return $modified_files;
  }

  /**
   * Process checking hashes of files from external URL.
   *
   * @param string $data
   *   Response data.
   * @param array $info
   *   Array of extension information.
   * @param \ArrayIterator $modified_files
   *   The list of modified files.
   */
  protected static function processHashes($data, array $info, \ArrayIterator $modified_files) {
    $directory_root = $info['install path'];
    if ($info['project'] === 'drupal') {
      $directory_root = '';
    }
    $module_path = drupal_get_path('module', 'automatic_updates');
    $key = file_get_contents($module_path . '/artifacts/keys/root.pub');
    $verifier = new Verifier($key);
    $files = $verifier->verifyCsigMessage($data);
    $checksums = new ChecksumList($files, TRUE);
    foreach (new FailedCheckumFilter($checksums, $directory_root) as $failed_checksum) {
      $file_path = implode(DIRECTORY_SEPARATOR, array_filter([
        $directory_root,
        $failed_checksum->filename,
      ]));
      if (!file_exists($file_path)) {
        $modified_files->append($file_path);
        continue;
      }
      $actual_hash = @hash_file(strtolower($failed_checksum->algorithm), $file_path);
      if ($actual_hash === FALSE || empty($actual_hash) || strlen($actual_hash) < 64 || strcmp($actual_hash, $failed_checksum->hex_hash) !== 0) {
        $modified_files->append($file_path);
      }
    }
  }

  /**
   * Get an iterator of extension name and hash URL.
   *
   * @param array $extensions
   *   The list of extensions, keyed by extension name and value the info array.
   *
   * @codingStandardsIgnoreStart
   *
   * @return \Generator
   *
   * @@codingStandardsIgnoreEnd
   */
  protected static function getHashRequests(array $extensions) {
    foreach ($extensions as $name => $info) {
      // We can't check for modifications if we don't know the version.
      if (!($info['version'])) {
        continue;
      }
      yield [$name => self::buildUrl($info)];
    }
  }

  /**
   * Build an extension's hash file URL.
   *
   * @param array $info
   *   The extension's info.
   *
   * @return string
   *   The URL endpoint with for an extension.
   */
  protected static function buildUrl(array $info) {
    $version = $info['version'];
    $project_name = $info['project'];
    $hash_name = self::getHashName($info);
    $uri = ltrim(variable_get('automatic_updates_hashes_uri', 'https://updates.drupal.org/release-hashes'), '/');
    return "$uri/$project_name/$version/$hash_name";
  }

  /**
   * Get the hash file name.
   *
   * @param array $info
   *   The extension's info.
   *
   * @return string|null
   *   The hash name.
   */
  protected static function getHashName(array $info) {
    $hash_name = 'contents-sha256sums';
    if ($info['packaged']) {
      $hash_name .= '-packaged';
    }
    return $hash_name . '.csig';
  }

}
