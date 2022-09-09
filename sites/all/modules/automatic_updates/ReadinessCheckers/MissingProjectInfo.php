<?php

/**
 * Missing project info checker.
 */
class MissingProjectInfo implements ReadinessCheckerInterface {
  use IgnoredPathsTrait;
  use ProjectInfoTrait;

  /**
   * {@inheritdoc}
   */
  public static function run() {
    return static::missingProjectInfoCheck();
  }

  /**
   * Check for projects missing project info.
   *
   * @return array
   *   An array of translatable strings if any checks fail.
   */
  protected static function missingProjectInfoCheck() {
    $messages = [];
    foreach (static::getInfos() as $extension_name => $info) {
      if (static::isIgnoredPath($info['install path'])) {
        continue;
      }
      if (!$info['version']) {
        $messages[] = t('The project "@extension" can not be updated because its version is either undefined or a dev release.', ['@extension' => $info['name']]);
      }
    }
    return $messages;
  }

}
