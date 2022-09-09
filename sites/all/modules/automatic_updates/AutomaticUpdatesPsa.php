<?php

/**
 * @file
 * Contains AutomaticUpdatesPsa class.
 */

use Composer\Semver\VersionParser;

/**
 * Class AutomaticUpdatesPsa.
 */
class AutomaticUpdatesPsa {

  const PSA_ENDPOINT = 'https://updates.drupal.org/psa.json';

  /**
   * {@inheritdoc}
   */
  public static function getPublicServiceMessages() {
    $messages = array();

    if (!variable_get('automatic_updates_enable_psa', TRUE)) {
      return $messages;
    }

    if ($cache = cache_get('automatic_updates_psa')) {
      $response = $cache->data;
    }
    else {
      $psa_endpoint = variable_get('automatic_updates_psa_endpoint', self::PSA_ENDPOINT);
      $response = drupal_http_request($psa_endpoint);
      if (isset($response->code) && ($response->code == 200)) {
        // Set response in cache for 12 hours.
        cache_set('automatic_updates_psa', $response, 'cache', variable_get('automatic_updates_check_frequency', REQUEST_TIME + 3600 * 12));
      }
      else {
        watchdog('automatic_updates', 'Drupal PSA endpoint %url is unreachable.', array('%url' => $psa_endpoint), WATCHDOG_ERROR);
        return array(t('Drupal PSA endpoint %url is unreachable.', array('%url' => $psa_endpoint)));
      }
    }

    $json_payload = json_decode($response->data, FALSE);
    if ($json_payload !== NULL) {
      foreach ($json_payload as $json) {
        try {
          if ($json->is_psa && ($json->type === 'core' || static::isValidExtension($json->type, $json->project))) {
            $messages[] = static::message($json->title, $json->link);
          }
          elseif ($json->type === 'core') {
            static::parseConstraints($messages, $json, VERSION);
          }
          elseif (static::isValidExtension($json->type, $json->project)) {
            static::contribParser($messages, $json);
          }
        }
        catch (\UnexpectedValueException $exception) {
          watchdog_exception('automatic_updates', $exception);
          $messages[] = t('Drupal PSA endpoint service is malformed.');
        }
      }
    }

    return $messages;
  }

  /**
   * Parse contrib project JSON version strings.
   *
   * @param array $messages
   *   The messages array.
   * @param object $json
   *   The JSON object.
   */
  protected static function contribParser(array &$messages, $json) {
    $extension_path = drupal_get_path($json->type, $json->project);
    $info = drupal_parse_info_file($extension_path . DIRECTORY_SEPARATOR . $json->project . '.info');
    $json->insecure = array_filter(array_map(static function ($version) {
      $version_array = explode('-', $version, 2);
      if ($version_array && $version_array[0] === DRUPAL_CORE_COMPATIBILITY) {
        return isset($version_array[1]) ? $version_array[1] : NULL;
      }
      if (count($version_array) === 1) {
        return $version_array[0];
      }
      if (count($version_array) === 2 && $version_array[1] === 'dev') {
        return $version;
      }
    }, $json->insecure));
    $version_array = explode('-', $info['version'], 2);
    $extension_version = isset($version_array[1]) && $version_array[1] !== 'dev' ? $version_array[1] : $info['version'];
    static::parseConstraints($messages, $json, $extension_version);
  }

  /**
   * Compare versions and add a message, if appropriate.
   *
   * @param array $messages
   *   The messages array.
   * @param object $json
   *   The JSON object.
   * @param string $current_version
   *   The current extension version.
   *
   * @throws \UnexpectedValueException
   */
  protected static function parseConstraints(array &$messages, $json, $current_version) {
    $version_string = implode('||', $json->insecure);
    if (empty($version_string)) {
      return;
    }
    $project_root = drupal_get_path('module', 'automatic_updates');
    require_once $project_root . DIRECTORY_SEPARATOR . 'vendor' . DIRECTORY_SEPARATOR . 'autoload.php';
    $parser = new VersionParser();
    $psa_constraint = $parser->parseConstraints($version_string);
    $contrib_constraint = $parser->parseConstraints($current_version);
    if ($psa_constraint->matches($contrib_constraint)) {
      $messages[] = static::message($json->title, $json->link);
    }
  }

  /**
   * Determine if extension exists and has a version string.
   *
   * @param string $extension_type
   *   The extension type i.e. module, theme.
   * @param string $project_name
   *   The project.
   *
   * @return bool
   *   TRUE if extension exists, else FALSE.
   */
  protected static function isValidExtension($extension_type, $project_name) {
    $extension_path = @drupal_get_path($extension_type, $project_name);
    if ($extension_path && ($info = drupal_parse_info_file($extension_path . DIRECTORY_SEPARATOR . $project_name . '.info'))) {
      return isset($info['version']);
    }
    return FALSE;
  }

  /**
   * Return a message.
   *
   * @param string $title
   *   The title.
   * @param string $link
   *   The link.
   *
   * @return string
   *   The PSA or SA message.
   */
  protected static function message($title, $link) {
    return format_string('<a href="@url">@message</a>', [
      '@message' => $title,
      '@url' => $link,
    ]);
  }

}
