<?php

/**
 * @file
 * mp_state_machine implementation of the StateMachine class.
 */

state_machine_load_class_file();

class MPStateFlow extends StateMachine {
  protected $entity_type;
  protected $type;
  protected $log;

  private $entity_types;
  // By default we want new revisions to be created.
  private $create_new_revisions = TRUE;

  public function __construct($entity_type, $object, $type) {
    $this->entity_type = $entity_type;
    $this->object = $object;
    $this->type = $type;

    if (!$this->ignore()) {
      $this->init();
      $this->set_current_state((string) $this->load());
    }
  }

  /**
   * Called from StateMachine::__construct to initialize the states and events.
   */
  public function init() {
    // Initialize states.
    $states = mp_state_machine_states_type($this->type);
    foreach ($states as $state_name => $options) {
      $this->create_state($state_name, $options);
    }

    $this->entity_types = mp_state_machine_entity_type_state_types();

    // Initialize events.
    $events = mp_state_machine_events_type($this->type);
    foreach ($events as $event_name => $options) {
      $this->create_event($event_name, $options);
    }
  }

  public function get_event($key) {
    if (!array_key_exists($key, $this->events)) {
      return FALSE;
    }

    if (is_array($this->events[$key])) {
      $options = $this->events[$key];
      $this->events[$key] = new MPStateFlow_Event($key, $this, $options);
    }

    return $this->events[$key];
  }

  public function get_object() {
    return $this->object;
  }

  public function get_states_options() {
    return $this->states;
  }

  public function get_entity_type() {
    return $this->entity_type;
  }

  public function get_state_type() {
    return $this->type;
  }

  /**
   * Get the label for the current state
   *
   * @return string
   */
  public function get_label_for_current_state() {
    $state_key = $this->get_current_state();
    $state = $this->get_state($state_key);
    if ($label = $state->get_option('label')) {
      return $label;
    }
    else {
      return drupal_ucfirst($state_key);
    }
  }

  /**
   * Extending fire_event() from state_machine's base.inc to add custom
   * processing.
   */
  public function fire_event($key, $log = '') {
    $event = $this->get_event($key);

    // Store the log message to make it available to the event class.
    $this->log = $log;

    if (!$event) {
      $this->error(t('Invalid event "@event_name" for state type "@state_type".', array(
        '@event_name' => $key,
        '@state_type' => $this->get_state_type(),
      )));
      return FALSE;
    }

    if ($new_state = $event->execute()) {
      // Allow the previous state to run its 'on_exit' callbacks.
      $this->get_state($this->get_current_state())->on_exit();

      // Set and save the new state.
      // This needs to be done before creating a new revision of an entity,
      // to make sure that the new state is available in hook_entity_update()
      // implementations. But we cannot persist it just yet, as it would save
      // a new record in the 'mp_state_machine' database table with the old
      // 'revision_id' value (as the new revision is created only below).
      $this->set_current_state($new_state);

      // First create new entity revision for this entity update if this entity
      // type supports revision, and there was no new revision created for this
      // update so far.
      if ($this->create_new_revisions && $this->supports_revisions() && empty($this->object->revision) && empty($this->object->is_new)) {
        $this->object->revision = TRUE;
        $this->object->log = !empty($log) ? $log : t('State machine: Updated %type state to %new_state.', array(
          '%type' => $this->type,
          '%new_state' => $new_state,
        ));
        entity_save($this->entity_type, $this->object);
      }

      // Now that we have a new revision created, we can persist the new state.
      $this->persist();

      // Allow the new state to run its 'on_enter' callbacks.
      $this->get_state($this->get_current_state())->on_enter();

      // Allow the event to "finish".
      $event->finish();

      // Allow other modules to react to the workflow transition.
      mp_state_machine_invoke_event_handlers($this, $key, $log);
    }
    else {
      $this->on_event_fail($event);
      return FALSE;
    }
  }

  public function persist() {
    list($entity_id, $revision_id, $bundle) = entity_extract_ids($this->entity_type, $this->object);

    $data = new stdClass();
    $data->entity_type = $this->entity_type;
    $data->entity_id = $entity_id;
    $data->revision_id = $this->get_revision_id();
    $data->type = $this->type;
    $data->state = $this->get_current_state();
    $data->timestamp = time();

    $update = ($this->supports_revisions() && $this->existing_revision($data->type, $data->entity_type, $data->entity_id, $data->revision_id)) ? array('entity_type', 'entity_id', 'revision_id', 'type') : array();
    return drupal_write_record('mp_state_machine', $data, $update);
  }

  public function load() {
    $state = FALSE;

    list($entity_id, $revision_id, $bundle) = entity_extract_ids($this->entity_type, $this->object);

    // If entity type supports revisions.
    if (!empty($this->supports_revisions())) {
      // Try to load the state for the current revision ID.
      if (!$state = $this->revision_state($this->entity_type, $entity_id, $revision_id, $this->type)) {
        // If not found, try to load the most recent state.
        $state = $this->latest_state($this->entity_type, $entity_id, $this->type);
      }
    }
    // If entity type doesn't support revisions.
    elseif (!empty($entity_id)) {
      $state = $this->latest_state($this->entity_type, $entity_id, $this->type);
    }

    return $state;
  }

  public function existing_revision($type, $entity_type, $entity_id, $revision_id) {
    $result = db_select('mp_state_machine', 'mpcs')
      ->fields('mpcs')
      ->condition('type', $type)
      ->condition('entity_type', $entity_type)
      ->condition('entity_id', $entity_id)
      ->condition('revision_id', $revision_id)
      ->countQuery()
      ->execute()
      ->fetchAll();
    return ($result[0]->expression) ? TRUE : FALSE;
  }

  public function revision_state($entity_type, $entity_id, $revision_id, $type) {
    $latest_state = db_query_range('
	  SELECT state
      FROM {mp_state_machine}
      WHERE type = :type
        AND entity_type = :entity_type
        AND entity_id = :entity_id
        AND revision_id = :revision_id', 0, 1,
      array(
        ':type' => $type,
        ':entity_type' => $entity_type,
        ':entity_id' => $entity_id,
        ':revision_id' => $revision_id,
      ))
      ->fetchCol('state');
    return !empty($latest_state[0]) ? $latest_state[0] : FALSE;
  }

  public function latest_state($entity_type, $entity_id, $type) {
    $latest_state = db_query_range('
      SELECT state
      FROM {mp_state_machine}
      WHERE type = :type
        AND entity_type = :entity_type
        AND entity_id = :entity_id
      ORDER BY id DESC', 0, 1,
      array(
        ':type' => $type,
        ':entity_type' => $entity_type,
        ':entity_id' => $entity_id,
      ))
      ->fetchCol('state');
    return !empty($latest_state[0]) ? $latest_state[0] : FALSE;
  }

  public function on_event_fail($event) {
    $event_name = array_search($event, $this->events);
    $this->error(t('Could not transition @entity_type @entity_id\'s "@state_type" using "@event_name" event.', array(
      '@entity_type' => $this->entity_type,
      '@entity_id' => $this->get_entity_id(),
      '@state_type' => $this->get_state_type(),
      '@event_name' => $event_name,
    )));
  }

  /**
   * Return event machine name to publish when skipping workflow
   *
   */
  public function skip_to_publish() {
    return 'publish';
  }

  public function supports_revisions() {
    $info = entity_get_info($this->entity_type);
    return (!empty($info['entity keys']['revision'])) ? TRUE : FALSE;
  }

  public function get_entity_id() {
    $info = entity_get_info($this->entity_type);
    return (!empty($this->object->{$info['entity keys']['id']})) ? $this->object->{$info['entity keys']['id']} : NULL;
  }

  public function get_revision_id() {
    $info = entity_get_info($this->entity_type);
    return ($this->supports_revisions() && !empty($this->object->{$info['entity keys']['revision']})) ? $this->object->{$info['entity keys']['revision']} : 0;
  }

  /**
   * Returns the event's log message.
   *
   * @return string
   *   The event's log message.
   */
  public function get_log() {
    return $this->log;
  }

  public function set_create_new_revisions($create_new_revisions = TRUE) {
    $this->create_new_revisions = $create_new_revisions;
  }

  public function get_create_new_revisions() {
    return $this->create_new_revisions;
  }

  protected function error($message, $code = 0) {
    watchdog('mp_state_machine', $message, array(), WATCHDOG_ERROR);
    throw new MPStateFlow_Exception(strip_tags($message), $code, $this);
  }
}

class MPStateFlow_Event extends StateMachine_Event {
  protected $original_state;

  public function __construct($key, $machine, $options = array()) {
    parent::__construct($key, $machine, $options);

    // Store the state of the object before transition.
    $this->original_state = $machine->get_current_state();
  }

  /**
   * @return MPStateFlow
   */
  public function get_machine() {
    return $this->machine;
  }

  public function validate() {
    if (parent::validate()) {
      if (!empty($this->options['permissions'])) {
        foreach ($this->options['permissions'] as $permission) {
          if (!user_access($permission)) {
            return FALSE;
          }
        }
      }
      return TRUE;
    }
    return FALSE;
  }

  /**
   * Allow the event to finish after the machine has changed state.
   */
  public function finish() {
    parent::finish();

    // Get the state machine and required information from it.
    $state_machine = $this->get_machine();
    $state_type = $state_machine->get_state_type();
    $object = $state_machine->get_object();
    $entity_type = $state_machine->get_entity_type();

    // If we updated a state of an entity that supports revisions, a new
    // revision has been created. Therefore we might need to save all other
    // state type entries for this entity in the database.
    if ($state_machine->supports_revisions()) {
      $entity_type_state_types = mp_state_machine_entity_type_state_types();
      if (count($entity_type_state_types[$entity_type] > 1)) {
        $other_state_types = array_diff(array_keys($entity_type_state_types[$entity_type]), array($state_type));
        foreach ($other_state_types as $other_state_type) {
          $other_state_machine = mp_state_machine_load($entity_type, $object, $other_state_type);
          $other_state_machine->persist();
        }
      }
    }

    // If it was the order that just got updated, we might want to also update
    // all its line item states.
    if ($entity_type == 'commerce_order') {
      $entity_type_state_types = mp_state_machine_entity_type_state_types();

      // We want to update the line item state only if it has the same state
      // type as the one that just got updated in the order.
      if (in_array($state_type, array_keys($entity_type_state_types['commerce_line_item'])) && !empty($object->commerce_line_items)) {
        $wrapper = entity_metadata_wrapper('commerce_order', $object);
        foreach ($wrapper->commerce_line_items as $line_item_wrapper) {
          // Load the state machine for this line item.
          $line_item_state_machine = mp_state_machine_load('commerce_line_item', $line_item_wrapper->value(), $state_type);
          // As the order should have a new revision just created, we do not
          // want to create new revisions for it for each updated line item.
          $line_item_state_machine->set_create_new_revisions(FALSE);

          // Only update line item's state if its current state is the same as
          // order's previous state. This is because the line item's state might
          // differ from the order's state, and in such situation we do not want
          // to update them automatically together with the order.
          if ($line_item_state_machine->get_current_state() == $this->original_state) {
            $line_item_state_machine->fire_event($this->key);
          }
        }
      }
    }

    // If it was line item that just got updated, we might want to create
    // a new order revision to keep track of this change.
    if ($entity_type == 'commerce_line_item') {
      $order = commerce_order_load($object->order_id);
      $order_state_machine = mp_state_machine_load('commerce_order', $order, $state_machine->get_state_type());

      // We want to create a new order revision only if it was just a single
      // line item update - not invoked from the order state update, as in such
      // case the revision has already been created.
      if ($state_machine->get_current_state() != $order_state_machine->get_current_state()) {

        // Create a new order revision if required.
        if ($state_machine->get_create_new_revisions()) {
          // Get the log message from the state machine.
          $log = $state_machine->get_log();

          $order->revision = TRUE;
          $order->log = !empty($log) ? $log : t('State machine: Updated line item @line_item_id %type state to %new_state.', array(
            '@line_item_id' => $object->line_item_id,
            '%type' => $state_machine->get_state_type(),
            '%new_state' => $state_machine->get_current_state(),
          ));
          commerce_order_save($order);
        }
      }
    }
  }
}

/**
 * A exception thrown by services and related modules when something goes
 * wrong.
 */
class MPStateFlow_Exception extends Exception {
  private $data;

  /**
   * Constructor for the ServicesException.
   *
   * @param string $message
   *  Error message.
   * @param int $code
   *  Optional. Error code. This often maps to the HTTP status codes. Defaults
   *  to 0.
   * @param mixed $data
   *  Information that can be used by the server to return information about
   *  the error.
   */
  public function __construct($message, $code = 0, $data = NULL) {
    parent::__construct($message, $code);

    $this->data = !empty($data) ? $data : $message;
  }

  /**
   * Returns the data associated with the exception.
   *
   * @return mixed
   */
  public function get_data() {
    return $this->data;
  }
}
