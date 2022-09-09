<?php

use Drupal\Signify\ChecksumList;
use Drupal\Signify\FailedCheckumFilter;
use Drupal\Signify\Verifier;

/**
 * Class to apply in-place updates.
 */
class InPlaceUpdate {
  use ProjectInfoTrait;

  /**
   * The manifest file that lists all file deletions.
   */
  const DELETION_MANIFEST = 'DELETION_MANIFEST.txt';

  /**
   * The directory inside the archive for file additions and modifications.
   */
  const ARCHIVE_DIRECTORY = 'files/';

  /**
   * The root file path.
   *
   * @var string
   */
  protected static $rootPath;

  /**
   * The folder where files are backed up.
   *
   * @var string
   */
  protected static $backup;

  /**
   * The temporary extract directory.
   *
   * @var string
   */
  protected static $tempDirectory;

  /**
   * {@inheritdoc}
   */
  public static function update($project_name, $project_type, $from_version, $to_version) {
    self::$rootPath = DRUPAL_ROOT;
    $project_root = drupal_get_path('module', 'automatic_updates');
    require_once $project_root . DIRECTORY_SEPARATOR . 'vendor' . DIRECTORY_SEPARATOR . 'autoload.php';
    // Bail immediately on updates if error category checks fail.
    if (ReadinessCheckerManager::getResults('error')) {
      return FALSE;
    }
    $success = FALSE;
    if ($project_name === 'drupal') {
      $project_root = self::$rootPath;
    }
    else {
      $project_root = drupal_get_path($project_type, $project_name);
    }
    if ($archive = self::getArchive($project_name, $from_version, $to_version)) {
      $modified = self::checkModifiedFiles($project_name, $archive);
      if (!$modified && self::backup($archive, $project_root)) {
        watchdog('automatic_updates', 'In place update has started.', [], WATCHDOG_INFO);
        try {
          $success = self::processUpdate($archive, $project_root);
          watchdog('automatic_updates', 'In place update has finished.', [], WATCHDOG_INFO);
        }
        catch (\Throwable $throwable) {
          watchdog('automatic_updates', 'In place update has failed.', [], WATCHDOG_ERROR);
          watchdog_exception($throwable);
        }
        catch (\Exception $exception) {
          watchdog('automatic_updates', 'In place update has failed.', [], WATCHDOG_ERROR);
          watchdog_exception($exception);
        }
        $result = automatic_updates_exec_command('updatedb:status');
        if (!empty($result['return_code'] || !empty($result['output']))) {
          // Rollback if there are database updates in the update.
          watchdog('automatic_updates', 'Database update exists', [], WATCHDOG_INFO);
          $success = FALSE;
        }
        if (!$success) {
          watchdog('automatic_updates', 'Rollback has started.', [], WATCHDOG_INFO);
          self::rollback($project_root);
          watchdog('automatic_updates', 'Rollback has finished.', [], WATCHDOG_INFO);
        }
        if ($success) {
          watchdog('automatic_updates', 'Cache clear has started.', [], WATCHDOG_INFO);
          self::cacheClear();
          watchdog('automatic_updates', 'Cache clear has finished.', [], WATCHDOG_INFO);
        }
      }
    }
    PostUpdateNotify::send($success, $project_name, $from_version, $to_version);
    return $success;
  }

  /**
   * Get an archive with the quasi-patch contents.
   *
   * @param string $project_name
   *   The project name.
   * @param string $from_version
   *   The current project version.
   * @param string $to_version
   *   The desired next project version.
   *
   * @return \ArchiverZip
   *   The archive.
   */
  protected static function getArchive($project_name, $from_version, $to_version) {
    $quasi_patch = self::getQuasiPatchFileName($project_name, $from_version, $to_version);
    $url = self::buildUrl($project_name, $quasi_patch);
    $temp_directory = drupal_realpath('temporary://') . DIRECTORY_SEPARATOR;
    $destination = file_destination($temp_directory . $quasi_patch, FILE_EXISTS_REPLACE);
    self::doGetResource($url, $destination);
    $csig_file = $quasi_patch . '.csig';
    $csig_url = self::buildUrl($project_name, $csig_file);
    $csig_destination = drupal_realpath(file_destination('temporary://' . $csig_file, FILE_EXISTS_REPLACE));
    self::doGetResource($csig_url, $csig_destination);
    $csig = file_get_contents($csig_destination);
    self::validateArchive($temp_directory, $csig);
    if (file_exists($destination)) {
      return new \ArchiverZip($destination);
    }
  }

  /**
   * Check if files are modified before applying updates.
   *
   * @param string $project_name
   *   The project name.
   * @param \ArchiverZip $archive
   *   The archive.
   *
   * @return bool
   *   Return TRUE if modified files exist, FALSE otherwise.
   */
  protected static function checkModifiedFiles($project_name, \ArchiverZip $archive) {
    $extensions = self::getInfos();
    try {
      $files = iterator_to_array(ModifiedFilesService::getModifiedFiles([$extensions[$project_name]]));
    }
    catch (\RuntimeException $exception) {
      watchdog_exception('automatic_updates', $exception);
      // While not strictly true there are modified files, we can't be sure
      // there aren't any. So assume the worst.
      return TRUE;
    }
    $files = array_unique($files);
    $archive_files = $archive->listContents();
    foreach ($archive_files as $index => &$archive_file) {
      $skipped_files = [
        self::DELETION_MANIFEST,
      ];
      // Skip certain files and all directories.
      if (in_array($archive_file, $skipped_files, TRUE) || substr($archive_file, -1) === '/') {
        unset($archive_files[$index]);
        continue;
      }
      self::stripFileDirectoryPath($archive_file);
    }
    unset($archive_file);
    if ($intersection = array_intersect($files, $archive_files)) {
      watchdog('automatic_updates', 'Can not update because %count files are modified: %paths', [
        '%count' => count($intersection),
        '%paths' => implode(', ', $intersection),
      ], WATCHDOG_ERROR);
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Perform retrieval of archive, with delay if archive is still being created.
   *
   * @param string $url
   *   The URL to retrieve.
   * @param string $destination
   *   The destination to download the archive.
   * @param null|int $delay
   *   The delay, defaults to NULL.
   */
  protected static function doGetResource($url, $destination, $delay = 0) {
    sleep($delay);
    $result = drupal_http_request($url);
    // Have to check for header directly as Core doesn't know HTTP 429.
    if ($result->code === '429' || isset($result->headers['retry-after'])) {
      $delay = $result->headers['retry-after'];
      self::doGetResource($url, $destination, $delay);
    }
    elseif ($result->code !== '200') {
      watchdog('automatic_updates', 'Retrieval of "@url" failed with: @message', [
        '@url' => $url,
        '@message' => $result->data,
      ], WATCHDOG_ERROR);
    }
    else {
      file_unmanaged_save_data($result->data, $destination, FILE_EXISTS_REPLACE);
    }
  }

  /**
   * Process update.
   *
   * @param \ArchiverZip $archive
   *   The archive.
   * @param string $project_root
   *   The project root directory.
   *
   * @return bool
   *   Return TRUE if update succeeds, FALSE otherwise.
   */
  protected static function processUpdate(\ArchiverZip $archive, $project_root) {
    $archive->extract(self::getTempDirectory());
    foreach (self::getFilesList(self::getTempDirectory()) as $file) {
      $file_real_path = self::getFileRealPath($file);
      $file_path = substr($file_real_path, strlen(self::getTempDirectory() . self::ARCHIVE_DIRECTORY));
      $project_real_path = self::getProjectRealPath($file_path, $project_root);
      $directory = dirname($project_real_path);
      file_prepare_directory($directory, FILE_CREATE_DIRECTORY);
      file_unmanaged_copy($file_real_path, $project_real_path, FILE_EXISTS_REPLACE);
      watchdog('automatic_updates', '"@file" was updated.', ['@file' => $project_real_path], WATCHDOG_INFO);
    }
    foreach (self::getDeletions() as $deletion) {
      $file_deletion = self::getProjectRealPath($deletion, $project_root);
      file_unmanaged_delete($file_deletion);
      watchdog('automatic_updates', '"@file" was deleted.', ['@file' => $file_deletion], WATCHDOG_INFO);
    }
    return TRUE;
  }

  /**
   * Validate the downloaded archive.
   *
   * @param string $directory
   *   The location of the downloaded archive.
   * @param string $csig
   *   The CSIG contents.
   *
   * @throws \SodiumException
   */
  protected static function validateArchive($directory, $csig) {
    $module_path = drupal_get_path('module', 'automatic_updates');
    $key = file_get_contents($module_path . '/artifacts/keys/root.pub');
    $verifier = new Verifier($key);
    $files = $verifier->verifyCsigMessage($csig);
    $checksums = new ChecksumList($files, TRUE);
    $failed_checksums = new FailedCheckumFilter($checksums, $directory);
    if (iterator_count($failed_checksums)) {
      throw new \RuntimeException('The downloaded files did not match what was expected.');
    }
  }

  /**
   * Backup before an update.
   *
   * @param \ArchiverZip $archive
   *   The archive.
   * @param string $project_root
   *   The project root directory.
   *
   * @return bool
   *   Return TRUE if backup succeeds, FALSE otherwise.
   */
  protected static function backup(\ArchiverZip $archive, $project_root) {
    $backup = file_create_filename('automatic_updates-backup', 'temporary://');
    file_prepare_directory($backup, FILE_CREATE_DIRECTORY);
    self::$backup = drupal_realpath($backup) . DIRECTORY_SEPARATOR;
    if (!self::$backup) {
      return FALSE;
    }
    foreach ($archive->listContents() as $file) {
      // Ignore files that aren't in the files directory.
      if (!self::stripFileDirectoryPath($file)) {
        continue;
      }
      $success = self::doBackup($file, $project_root);
      if (!$success) {
        return FALSE;
      }
    }
    $archive->extract(self::getTempDirectory(), [self::DELETION_MANIFEST]);
    foreach (self::getDeletions() as $deletion) {
      $success = self::doBackup($deletion, $project_root);
      if (!$success) {
        return FALSE;
      }
    }
    return TRUE;
  }

  /**
   * Remove the files directory path from files from the archive.
   *
   * @param string $file
   *   The file path.
   *
   * @return bool
   *   TRUE if path was removed, else FALSE.
   */
  protected static function stripFileDirectoryPath(&$file) {
    if (strpos($file, self::ARCHIVE_DIRECTORY) === 0) {
      $file = substr($file, 6);
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Execute file backup.
   *
   * @param string $file
   *   The file to backup.
   * @param string $project_root
   *   The project root directory.
   *
   * @return bool
   *   Return TRUE if backup succeeds, FALSE otherwise.
   */
  protected static function doBackup($file, $project_root) {
    $directory = self::$backup . dirname($file);
    if (!file_exists($directory) && !drupal_mkdir($directory, NULL, TRUE)) {
      return FALSE;
    }
    $project_real_path = self::getProjectRealPath($file, $project_root);
    if (file_exists($project_real_path) && !is_dir($project_real_path)) {
      $success = file_unmanaged_copy($project_real_path, self::$backup . $file, FILE_EXISTS_REPLACE);
      if (!$success) {
        return FALSE;
      }
      watchdog('automatic_updates', '"@file" was backed up in preparation for an update.', ['@file' => $project_real_path], WATCHDOG_INFO);
    }
    return TRUE;
  }

  /**
   * Rollback after a failed update.
   *
   * @param string $project_root
   *   The project root directory.
   */
  protected static function rollback($project_root) {
    if (!self::$backup) {
      return;
    }
    foreach (self::getFilesList(self::getTempDirectory()) as $file) {
      $file_real_path = self::getFileRealPath($file);
      $file_path = substr($file_real_path, strlen(self::getTempDirectory() . self::ARCHIVE_DIRECTORY));
      $project_real_path = self::getProjectRealPath($file_path, $project_root);
      $success = file_unmanaged_delete($project_real_path);
      if ($success) {
        watchdog('automatic_updates', '"@file" was successfully removed during rollback.', ['@file' => $project_real_path], WATCHDOG_INFO);
      }
      else {
        watchdog('automatic_updates', '"@file" failed removal on rollback.', ['@file' => $project_real_path], WATCHDOG_ERROR);
      }
    }
    foreach (self::getFilesList(self::$backup) as $file) {
      self::doRestore($file, $project_root);
    }
  }

  /**
   * Do restore.
   *
   * @param \SplFileInfo $file
   *   File to restore.
   * @param string $project_root
   *   The project root directory.
   */
  protected static function doRestore(\SplFileInfo $file, $project_root) {
    $file_real_path = self::getFileRealPath($file);
    $file_path = substr($file_real_path, strlen(self::$backup));
    $success = file_unmanaged_copy($file_real_path, self::getProjectRealPath($file_path, $project_root), FILE_EXISTS_REPLACE);
    if ($success) {
      watchdog('automatic_updates', '"@file" was successfully restored.', ['@file' => $file_path], WATCHDOG_INFO);
    }
    else {
      watchdog('automatic_updates', '"@file" failed restoration during rollback.', ['@file' => $file_path], WATCHDOG_ERROR);
    }
  }

  /**
   * Provide a recursive list of files, excluding directories.
   *
   * @param string $directory
   *   The directory to recurse for files.
   *
   * @return \RecursiveIteratorIterator|\SplFileInfo[]
   *   The iterator of SplFileInfos.
   */
  protected static function getFilesList($directory) {
    $filter = static function ($file, $file_name, $iterator) {
      /** @var \SplFileInfo $file */
      /** @var string $file_name */
      /** @var \RecursiveDirectoryIterator $iterator */
      if ($iterator->hasChildren() && $file->getFilename() !== '.git') {
        return TRUE;
      }
      $skipped_files = [
        self::DELETION_MANIFEST,
      ];
      return $file->isFile() && !in_array($file->getFilename(), $skipped_files, TRUE);
    };

    $innerIterator = new \RecursiveDirectoryIterator($directory, \RecursiveDirectoryIterator::SKIP_DOTS);
    return new \RecursiveIteratorIterator(new \RecursiveCallbackFilterIterator($innerIterator, $filter));
  }

  /**
   * Build a project quasi-patch download URL.
   *
   * @param string $project_name
   *   The project name.
   * @param string $file_name
   *   The file name.
   *
   * @return string
   *   The URL endpoint with for an extension.
   */
  protected static function buildUrl($project_name, $file_name) {
    $uri = ltrim(variable_get('automatic_updates_download_uri', 'https://www.drupal.org/in-place-updates'), '/');
    return "$uri/$project_name/$file_name";
  }

  /**
   * Get the quasi-patch file name.
   *
   * @param string $project_name
   *   The project name.
   * @param string $from_version
   *   The current project version.
   * @param string $to_version
   *   The desired next project version.
   *
   * @return string
   *   The quasi-patch file name.
   */
  protected static function getQuasiPatchFileName($project_name, $from_version, $to_version) {
    return "$project_name-$from_version-to-$to_version.zip";
  }

  /**
   * Get file real path.
   *
   * @param \SplFileInfo $file
   *   The file to retrieve the real path.
   *
   * @return string
   *   The file real path.
   */
  protected static function getFileRealPath(\SplFileInfo $file) {
    $real_path = $file->getRealPath();
    if (!$real_path) {
      throw new \RuntimeException(sprintf('Could not get real path for "%s"', $file->getFilename()));
    }
    return $real_path;
  }

  /**
   * Get the real path of a file.
   *
   * @param string $file_path
   *   The file path.
   * @param string $project_root
   *   The project root directory.
   *
   * @return string
   *   The real path of a file.
   */
  protected static function getProjectRealPath($file_path, $project_root) {
    return rtrim($project_root, '/\\') . DIRECTORY_SEPARATOR . $file_path;
  }

  /**
   * Provides the temporary extraction directory.
   *
   * @return string
   *   The temporary directory.
   */
  protected static function getTempDirectory() {
    if (!self::$tempDirectory) {
      self::$tempDirectory = file_create_filename('automatic_updates-update', 'temporary://');
      file_prepare_directory(self::$tempDirectory, FILE_CREATE_DIRECTORY);
      self::$tempDirectory = drupal_realpath(self::$tempDirectory) . DIRECTORY_SEPARATOR;
    }
    return self::$tempDirectory;
  }

  /**
   * Get an iterator of files to delete.
   *
   * @return \ArrayIterator
   *   Iterator of files to delete.
   */
  protected static function getDeletions() {
    $deletions = [];
    if (!file_exists(self::getTempDirectory() . self::DELETION_MANIFEST)) {
      return new \ArrayIterator($deletions);
    }
    $handle = fopen(self::getTempDirectory() . self::DELETION_MANIFEST, 'r');
    if ($handle) {
      while (($deletion = fgets($handle)) !== FALSE) {
        if ($result = trim($deletion)) {
          $deletions[] = $result;
        }
      }
      fclose($handle);
    }
    return new \ArrayIterator($deletions);
  }

  /**
   * Clear cache on successful update.
   */
  protected static function cacheClear() {
    if (function_exists('opcache_reset')) {
      opcache_reset();
    }
    automatic_updates_exec_command('cache:clear');
  }

}
