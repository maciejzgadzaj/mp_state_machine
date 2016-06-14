<?php

/**
 * @file
 * MP Commerce implementation of the State Machine class.
 */

state_machine_load_class_file();

class MPStateFlow extends StateMachine {
  protected $entity_type;
  protected $type;

  private $entity_types;

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

    $this->entity_types = mp_state_machine_entity_types();

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
   * Extending fire_event() from state_machine's base.inc to add uid and log
   * arguments.
   */
  public function fire_event($key, $uid = NULL, $log = '') {
    $event = $this->get_event($key);

    if ($event && ($new_state = $event->execute())) {
      // Allow the previous state to run its 'on_exit' callbacks.
      $this->get_state($this->get_current_state())->on_exit();

      // First create new entity revision for this entity update if this entity
      // type supports revision, and there was no new revision created for this
      // update so far.
      if ($this->supports_revisions() && empty($this->object->revision) && empty($this->object->is_new)) {
        $this->object->revision = TRUE;
        $this->object->log = t('State machine: Updated %type state to %new_state.', array(
          '%type' => $this->type,
          '%new_state' => $new_state,
        ));
        entity_save($this->entity_type, $this->object);
      }

      // Set and save the new state.
      $this->set_current_state($new_state);
      $this->persist();

      // Allow the new state to run its 'on_enter' callbacks.
      $this->get_state($this->get_current_state())->on_enter();

      // Allow the event to "finish".
      $event->finish();

      // Allow state_flow to provide other hooks or event triggers.
      // @TODO.
//      state_flow_invoke_event_handlers($this, $key, $uid, $log);
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

  public function set_node_revision() {
    $vid = $this->get_latest_revision($this->object->nid);
    if (!empty($vid) && ($vid != $this->object->vid)) {
      $rev_state_rec = $this->revision_state_record($this->object->nid, $this->object->vid);
      state_flow_promote_node_revision($rev_state_rec, $this->object->nid, $this->object->vid);
    }
    $result = db_update('node_revision')
        ->fields(array(
          'status' => 1,
        ))
        ->condition('vid', $vid)
        ->execute();
  }

//  public function set_principle_revision() {
//    $nid = $this->object->nid;
//    $vid = $this->get_latest_revision($nid);
//    $result = db_update('node_revision_states')
//      ->fields(array(
//        'status' => 0,
//      ))
//      ->condition('nid', $nid)
//      ->condition('vid', $vid, '!=')
//      ->execute();
//  }

//  public function get_latest_revision($nid) {
//    $result = db_query('SELECT MAX(vid) FROM {node_revision} WHERE nid = :nid', array(':nid' => $nid))->fetchCol('vid');
//    return !empty($result[0]) ? $result[0] : FALSE;
//  }

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
    $key = array_search($event, $this->events);
    watchdog('mp_state_machine', 'Could not transition %entity_type %entity_id using %event event.', array(
      '%entity_type' => $this->entity_type,
      '%entity_id' => $this->get_entity_id(),
      '%event' => $key,
    ), WATCHDOG_ERROR);
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
}

class MPStateFlow_Event extends StateMachine_Event {

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

    $state_machine = $this->get_machine();
    
    if ($state_machine->get_entity_type() == 'commerce_order') {
      $order = $state_machine->get_object();
      $state_type = $state_machine->get_state_type();
      $entity_type_state_types = mp_state_machine_entity_types();
      if (in_array($state_type, $entity_type_state_types['commerce_line_item']) && !empty($order->commerce_line_items)) {
        $wrapper = entity_metadata_wrapper('commerce_order', $order);
        foreach ($wrapper->commerce_line_items as $line_item_wrapper) {
          $line_item_state_machine = mp_state_machine_load('commerce_line_item', $line_item_wrapper->value(), $state_type);
          if ($line_item_state_machine->get_current_state() == $order->original->status) {
            $line_item_state_machine->fire_event($this->key);
          }
        }
      }
    }
    
    if ($state_machine->get_entity_type() == 'commerce_line_item') {
      $line_item = $state_machine->get_object();
      $order = commerce_order_load($line_item->order_id);
      $order_state_machine = mp_state_machine_load('commerce_order', $order, $state_machine->get_state_type());
      if ($state_machine->get_current_state() != $order_state_machine->get_current_state()) {
        $order->revision = TRUE;
        $order->log = t('State machine: Updated line item @line_item_id %type state to %new_state.', array(
          '@line_item_id' => $line_item->line_item_id,
          '%type' => $state_machine->get_state_type(),
          '%new_state' => $state_machine->get_current_state(),
        ));
        commerce_order_save($order);
      }
    }
  }
}
