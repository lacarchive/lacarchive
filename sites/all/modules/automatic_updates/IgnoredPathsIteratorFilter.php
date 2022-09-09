<?php

/**
 * Provides an iterator filter for file paths which are ignored.
 */
class IgnoredPathsIteratorFilter extends \FilterIterator {
  use IgnoredPathsTrait;

  /**
   * {@inheritdoc}
   */
  public function accept() {
    return self::isIgnoredPath($this->current());
  }

}
