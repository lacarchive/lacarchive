<?php
/**
 * @file
 * Contains merge process for invoking merge hooks.
 */

class EntityMergeProcessSaveTarget extends EntityMergeProcessBase {

  /**
   * Overrides EntityMergeProcessBase::run()
   */
  public function run(&$context = array()) {
    if (!entity_load_unchanged($this->entityType, entity_id($this->entityType, $this->targetEntity))) {
      $this->parentProcess->cancel();
    }
    else {
      entity_save($this->entityType, $this->targetEntity);
    }
  }

  /**
   * {@inheritdoc}
   */
  public function getProgressMessage(&$context = array()) {
    return t('Saved Merged @entity', array('@entity' => entity_get_info($this->entityType)['label']));
  }
}
