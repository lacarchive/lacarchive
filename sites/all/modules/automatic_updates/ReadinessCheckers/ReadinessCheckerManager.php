<?php

/**
 * Defines a chained readiness checker implementation combining multiple checks.
 */
class ReadinessCheckerManager {

  /**
   * An unsorted array of active checkers.
   *
   * The keys are category, next level is integers that indicate priority.
   * Values are arrays of ReadinessCheckerInterface objects.
   *
   * @var ReadinessCheckerInterface[][][]
   */
  protected static $checkers = [];

  /**
   * Get checkers.
   *
   * @return array
   *   The registered checkers.
   */
  protected static function getCheckers() {
    static::$checkers['warning'][0][] = 'BlacklistPhp72Versions';
    static::$checkers['warning'][0][] = 'CronFrequency';
    static::$checkers['warning'][0][] = 'FileOwnership';
    static::$checkers['warning'][0][] = 'MissingProjectInfo';
    static::$checkers['warning'][0][] = 'ModifiedFiles';
    static::$checkers['warning'][0][] = 'PhpSapi';
    static::$checkers['error'][0][] = 'DiskSpace';
    static::$checkers['error'][0][] = 'OpcodeCache';
    static::$checkers['error'][0][] = 'PendingDbUpdates';
    static::$checkers['error'][0][] = 'ReadOnlyFilesystem';

    return static::$checkers;
  }

  /**
   * Run checks.
   *
   * @param string $category
   *   The category of check.
   *
   * @return array
   *   An array of translatable strings.
   */
  public static function run($category) {
    $messages = [];
    if (!static::isEnabled()) {
      return $messages;
    }
    if (!isset(static::getSortedCheckers()[$category])) {
      throw new \InvalidArgumentException(sprintf('No readiness checkers exist of category "%s"', $category));
    }
    foreach (static::getSortedCheckers()[$category] as $checker) {
      $messages[] = $checker::run();
    }
    $messages = array_merge(...$messages);
    // Guard against variable_set stampede by checking if the values have
    // changed since previous run.
    $previous_messages = variable_get("automatic_updates.readiness_check_results.$category");
    if ($previous_messages !== $messages) {
      variable_set("automatic_updates.readiness_check_results.$category", $messages);
    }
    if (variable_get('automatic_updates.readiness_check_timestamp') !== REQUEST_TIME) {
      variable_set('automatic_updates.readiness_check_timestamp', REQUEST_TIME);
    }
    return $messages;
  }

  /**
   * {@inheritdoc}
   */
  public static function getResults($category) {
    $results = [];
    if (static::isEnabled()) {
      $results = variable_get("automatic_updates.readiness_check_results.$category", []);
    }
    return $results;
  }

  /**
   * {@inheritdoc}
   */
  public static function timestamp() {
    $last_check_timestamp = variable_get('automatic_updates.readiness_check_timestamp');
    if (!is_numeric($last_check_timestamp)) {
      $last_check_timestamp = variable_get('install_time', 0);
    }
    return $last_check_timestamp;
  }

  /**
   * {@inheritdoc}
   */
  public static function isEnabled() {
    return variable_get('automatic_updates_enable_readiness_checks', TRUE);
  }

  /**
   * {@inheritdoc}
   */
  public static function getCategories() {
    return ['error', 'warning'];
  }

  /**
   * Sorts checkers according to priority.
   *
   * @return ReadinessCheckerInterface[]
   *   A sorted array of checker objects.
   */
  protected static function getSortedCheckers() {
    $sorted = [];
    foreach (static::getCheckers() as $category => $priorities) {
      foreach ($priorities as $checkers) {
        krsort($checkers);
        $sorted[$category][] = $checkers;
      }
      $sorted[$category] = array_unique(array_merge(...$sorted[$category]));
    }
    return $sorted;
  }

}
