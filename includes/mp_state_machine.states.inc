<?php

/**
 * @file
 * Contains pre-defined state types, states and events for the mp_state_machine.
 */

define('MP_STATE_MACHINE_CHECKOUT_STATE', 'checkout_state');
define('MP_STATE_MACHINE_PAYMENT_STATE', 'payment_state');

/**
 * Returns an array of state types.
 *
 * @return array
 *   An array of state types.
 */
function mp_state_machine_types() {
  $types = array(
    // Duplicate standard commerce_order statuses to the commerce_line_items.
    MP_STATE_MACHINE_CHECKOUT_STATE => array(
      'label' => t('Checkout state'),
      'description' => t('The current state of the checkout process.'),
      'entity_types' => array('commerce_order', 'commerce_line_item'),
    ),
    MP_STATE_MACHINE_PAYMENT_STATE => array(
      'label' => t('Payment state'),
      'description' => t('The current state of the payment process.'),
      'entity_types' => array('commerce_order', 'commerce_line_item'),
    ),
  );

  // Allow other modules to alter pre-defined state types.
  drupal_alter('mp_state_machine_types', $types);

  return $types;
}

/**
 * Returns an array of state types keyed by entity type.
 *
 * @return array
 *   An array of state types keyed by entity type.
 */
function mp_state_machine_entity_type_state_types() {
  $entity_types = array();

  $types = mp_state_machine_types();
  foreach ($types as $type_name => $type_info) {
    if (!empty($type_info['entity_types'])) {
      foreach ($type_info['entity_types'] as $entity_type) {
        $entity_types[$entity_type][$type_name] = $type_info;
      }
    }
  }

  return $entity_types;
}

/**
 * Returns an array of all state definitions.
 *
 * @return array
 *   An array of all state definitions.
 */
function mp_state_machine_states() {
  $states = array();

  // Standard commerce_order statuses.
  foreach (commerce_order_statuses() as $name => $status) {
    $states[MP_STATE_MACHINE_CHECKOUT_STATE][$name] = array(
      'label' => $status['title'],
    );
  }

  // Payment-specific statuses.
  $states[MP_STATE_MACHINE_PAYMENT_STATE] = array(
    'unpaid' => array(
      'label' => 'Unpaid',
    ),
    'initialized' => array(
      'label' => 'Initialized',
    ),
    'completed' => array(
      'label' => 'Completed',
    ),
    'validated' => array(
      'label' => 'Validated',
    ),
    'refused' => array(
      'label' => 'Refused',
    ),
  );

  // Allow other modules to alter pre-defined states.
  drupal_alter('mp_state_machine_states', $states);

  return $states;
}

/**
 * Returns an array of state definitions of a specific type.
 *
 * @param string $type
 *   A state type to return the state definitions for.
 *
 * @return array|mixed
 *   An array of state definitions of a specific type.
 */
function mp_state_machine_states_type($type) {
  $states = mp_state_machine_states();
  return (isset($states[$type])) ? $states[$type] : array();
}

/**
 * Returns an array of all event definitions.
 *
 * @return array
 *   An array of all event definitions.
 */
function mp_state_machine_events() {
  $events = array();

  $order_statuses = commerce_order_statuses();
  foreach ($order_statuses as $order_status_name => $order_status) {
    $events[MP_STATE_MACHINE_CHECKOUT_STATE]['to_' . $order_status_name] = array(
      'label' => t('To !title', array('!title' => $order_status['title'])),
      'target' => $order_status['name'],
      'origin' => array_keys($order_statuses),
    );
  }

  // This is based on Hipay TPP payment method integration. The idea is:
  // 1) After making Hipay initialization call, the payment state should be
  //    changed to 'initialized'.
  // 2) After returning from payment offsite redirect, the payment state
  //    should be changed to 'completed' (or 'refused').
  // 3) After receiving asyncronous server-to-server notification, the payment
  //    state should be changed to 'validated' (or 'refused').
  // It might happen though that the server-to-server notificaton is received
  // *before* the user returns from the offsite redirect (when the payment state
  // is still 'initialized') - therefore we need to allow transitions to
  // 'validated' and 'refused' states from 'initialized' as well, even though
  // there is 'completed' state in-between.
  $events[MP_STATE_MACHINE_PAYMENT_STATE] = array(
    'to_initialized' => array(
      'label' => t('To Initialized'),
      'target' => 'initialized',
      'origin' => 'unpaid',
    ),
    'to_completed' => array(
      'label' => t('To Completed'),
      'target' => 'completed',
      'origin' => 'initialized',
    ),
    'to_validated' => array(
      'label' => t('To Validated'),
      'target' => 'validated',
      'origin' => array('initialized', 'completed'),
    ),
    'to_refused' => array(
      'label' => t('To Refused'),
      'target' => 'refused',
      'origin' => array('initialized', 'completed'),
    ),
  );

  // Allow other modules to alter pre-defined events.
  drupal_alter('mp_state_machine_events', $events);

  return $events;
}

/**
 * Returns an array of event definitions for a specific state type.
 *
 * @param string $type
 *   A state type to return the event definitions for.
 *
 * @return array
 *   An array of event definitions for a specific state type.
 */
function mp_state_machine_events_type($type) {
  $events = mp_state_machine_events();
  return (isset($events[$type])) ? $events[$type] : array();
}

/**
 * Returns an instantiated state machine instance for the given data.
 *
 * @param string $entity_type
 *   An entity type to return the state machine for.
 * @param object $entity
 *   An entity object to return the state machine for.
 * @param string $state_type
 *   A state type to return the state machine for.
 * @param bool $reset
 *   A boolean indicating whether to return a static cached version
 *   of the state machine or instantiate a new instance.
 *
 * @return MPStateFlow
 *   An instantiated state machine instance for the given data.
 */
function mp_state_machine_load($entity_type, $entity, $state_type, $reset = FALSE) {
  $objects = &drupal_static(__FUNCTION__);

  list($entity_id, $vid, $bundle) = entity_extract_ids($entity_type, $entity);

  if (!isset($objects[$entity_type][$entity_id][$state_type]) || $reset) {
    $objects[$entity_type][$entity_id][$state_type] = new MPStateFlow($entity_type, $entity, $state_type);
  }

  return $objects[$entity_type][$entity_id][$state_type];
}
