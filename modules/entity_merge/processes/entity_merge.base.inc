<?php
/**
 * @file
 * Contains base class for entity merge processes.
 */

/**
 * Base class for entity merge processes.
 */
abstract class EntityMergeProcessBase implements EntityMergeProcessInterface {

  /**
   * The entity type being merged.
   * @var string
   */
  protected $entityType;

  /**
   * The current queue.
   */
  protected $queue;

  /**
   * The parent process
   */
  public $parentProcess;

  /**
   * The sub processes
   */
  protected $subProcesses = array();

  /**
   * The current sub process.
   */
  protected $currentSubProcess = FALSE;

  /**
   * The first entity to be merged.
   */
  protected $entity1;

  /**
   * The second entity to be merged.
   */
  protected $entity2;

  /**
   * The id in the semaphore table.
   */
  protected $id;

  /**
   * The context of the operation.
   */
  private $context = array();

  public function __construct($entity_type, $entity1, $entity2, $queue = FALSE, $target_entity = FALSE, $parent = FALSE, $conf = array()) {
    $this->entityType = $entity_type;
    $this->entity1 = $entity1;
    $this->entity2 = $entity2;
    $this->queue = $queue;
    $this->targetEntity = $target_entity;
    $this->parentProcess = $parent;
    $this->conf = $conf;
  }

  /**
   * Get the context.
   */
  public function getContext() {
    return $this->context;
  }

  /**
   * Set the context.
   */
  public function setContext(array $context) {
    $this->context = $context;
  }

  /**
   * Set the QueueObject
   */
  public function setQueue($queue) {
    $this->queue = $queue;
  }

  /**
   * Register subprocesses.
   *
   * @return array
   *  An array keyed by a unique string and containing atleast
   *  - class: The class used to run the sub process.
   *  - weight: The weight. Sub processes will be done in order of weight.
   */
  public function registerSubProcesses() {
    return array();
  }

  /**
   * Prepare sub-processes.
   */
  public function prepareSubProcesses($processes) {
    foreach ($processes as $name => $proc) {
      if (empty($proc['class']) || !class_exists($proc['class'])) {
        throw new Exception('Class ' . $proc['class'] . ' was not found');
      }

      $class = $proc['class'];
      $entityType = isset($proc['entityType']) ? $proc['entityType'] : $this->entityType;
      $entity1 = isset($proc['entity1']) ? $proc['entity1'] : $this->entity1;
      $entity2 = isset($proc['entity2']) ? $proc['entity2'] : $this->entity2;
      $targetEntity = isset($proc['targetEntity']) ? $proc['targetEntity'] : $this->targetEntity;

      // Get the config to pass onto the sub-process.
      $skip_keys = array('class', 'entityType', 'entity1', 'entity2', 'targetEntity');
      $conf = array_diff_key($proc, array_fill_keys($skip_keys, TRUE));

      // Merge in the parent config.
      $conf += $this->conf;

      $this->subProcesses[$name] = new $class($entityType, $entity1, $entity2, $this->queue, $targetEntity, $this, $conf);
    }
  }

  /**
   * Prepare this process.
   */
  public function prepare() {
    $processes = $this->registerSubProcesses();
    $this->prepareSubProcesses($processes);
    return $this;
  }

  /**
   * Implements EntityMergeProcessIntergace::start();
   */
  public function go() {
    if (!empty($this->subProcesses)) {
      $this->queueNextProcess();
      return;
    }

    $context = $this->getContext();
    $this->run($context);

    if (isset($context['sandbox']['finished']) && $context['sandbox']['finished'] != 1) {
      $this->setContext($context);
      $this->queue->createItem($this);
    }
    else {
      $this->finish();
    }
  }

  /**
   * Run the merge process for a batch.
   */
  public function batchGo(&$context = array()) {
    if (!isset($context['results'])) {
      $context['results'] = array();
    }

    $context['results'] += array(
      'targetEntity' => $this->targetEntity,
      'entity1' => $this->entity1,
      'entity2' => $this->entity2,
    );

    $this->targetEntity = $context['results']['targetEntity'];
    $this->entity1 = $context['results']['entity1'];
    $this->entity2 = $context['results']['entity2'];

    if (!empty($this->subProcesses)) {
      $operations = array();
      foreach ($this->subProcesses as $subProcess) {
        $operations[] = array('entity_merge_form_batch_process', array($subProcess));
      }
      $operations[] = array('entity_merge_form_batch_process_run', array($this));
      $batch = array(
        'operations' => $operations,
        'title' => t('Processing Merge'),
        'file' => drupal_get_path('module', 'entity_merge').'/entity_merge.batch.inc',
      );
      batch_set($batch);
    }
    else {
      $this->run($context);
    }
  }

  /**
   * Just run the process in a batch context.
   */
  public function batchRun(&$context = array()) {
    if (!isset($context['results'])) {
      $context['results'] = array();
    }

    $context['results'] += array(
      'targetEntity' => $this->targetEntity,
      'entity1' => $this->entity1,
      'entity2' => $this->entity2,
    );

    $this->targetEntity = $context['results']['targetEntity'];
    $this->entity1 = $context['results']['entity1'];
    $this->entity2 = $context['results']['entity2'];

    $this->run($context);
  }

  /**
   * Run Method.
   */
  public function run(&$context = array()) {
    $context['sandbox']['message'] = $this->getProgressMessage($context);
  }

  /**
   * Get the progress message.
   */
  public function getProgressMessage(&$context = array()) {
    return t('Processing @class', array('@class' => get_class($this)));
  }

  /**
   * Finish running. Pass off control to the parent.
   */
  public function finish() {
    if (!empty($this->parentProcess)) {
      if (!$this->parentProcess->queueNextProcess()) {
        $this->parentProcess->finish();
      }
    }
  }

  /**
   * Cancel the process.
   */
  public function cancel() {
    if (!empty($this->parentProcess)) {
      $this->parentProcess->notifyCancellation($this);
    }

    $this->subProcesses = array();
    $this->finish();
  }

  /**
   * Notify of a cancellation.
   */
  public function notifyCancellation($process) {
  }

  /**
   * Queue the next process
   */
  public function queueNextProcess() {
    if ($this->currentSubProcess === FALSE) {
      $this->currentSubProcess = key($this->subProcesses);
    }
    else {
      $next = FALSE;
      $assigned = FALSE;
      foreach(array_keys($this->subProcesses) as $key) {
        if ($next) {
          $this->currentSubProcess = $key;
          $assigned = TRUE;
          break;
        }
        if ($key == $this->currentSubProcess) {
          $next = TRUE;
        }
      }
      if (!$assigned) {
        return FALSE;
      }
    }

    // Actually Queue the process.
    $this->queue->createItem($this->subProcesses[$this->currentSubProcess]);
    return TRUE;
  }
}
