<?php

/**
 * Batch process for entity merges.
 */
function entity_merge_form_batch_process($handler, &$context) {
  $handler->prepare()->batchGo($context);
}

/**
 * Batch process to just run entity merges.
 */
function entity_merge_form_batch_process_run($handler, &$context) {
  $handler->batchRun($context);
}
