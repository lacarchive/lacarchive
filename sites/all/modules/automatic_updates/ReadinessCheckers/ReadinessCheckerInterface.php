<?php

/**
 * Interface for objects capable of readiness checking.
 */
interface ReadinessCheckerInterface {

  /**
   * Run check.
   *
   * @return array
   *   An array of translatable strings.
   */
  public static function run();

}
