<?php

/**
 * Returns an array of state types.
 *
 * @return array
 *   An array of state types.
 */
function mp_state_machine_types() {
  return array(
    // Duplicate standard commerce_order statuses to the commerce_line_items.
    'status' => array(
      'label' => t('Status'),
      'description' => t('The current status of the line item.'),
      'entity_types' => array('commerce_line_item'),
    ),
    'payment_status' => array(
      'label' => t('Payment status'),
      'description' => t('The current status of the payment transaction.'),
      'entity_types' => array('commerce_order', 'commerce_line_item'),
    ),
  );
}

function mp_state_machine_entity_types() {
  $entity_types = array();

  $types = mp_state_machine_types();
  foreach ($types as $type_name => $type_info) {
    if (!empty($type_info['entity_types'])) {
      foreach ($type_info['entity_types'] as $entity_type) {
        $entity_types[$entity_type][$type_name] = $type_name;
      }
    }
  }

  return $entity_types;
}

/**
 * Returns an array of all defined states.
 *
 * @return array
 *   An array of all defined states.
 */
function mp_state_machine_states() {
  $states = array();

  // Standard commerce_order statuses.
  foreach (commerce_order_statuses() as $name => $status) {
    $states['status'][$name] = array(
      'label' => $status['title'],
    );
  }

  // Payment-specific statuses.
  $states['payment_status'] = array(
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

  return $states;
}

/**
 * Returns an array of states of a specific type.
 *
 * @param string $type
 *   A type to returns the states for.
 *
 * @return array|mixed
 *   An array of states of a specific type.
 */
function mp_state_machine_states_type($type) {
  $states = mp_state_machine_states();
  return (isset($states[$type])) ? $states[$type] : array();
}

/**
 * Returns an array of all defined events.
 *
 * @return array
 *   An array of all defined events.
 */
function mp_state_machine_events() {
  $events = array();

  $order_statuses = commerce_order_statuses();
  foreach ($order_statuses as $order_status_name => $order_status) {
    $events['status']['to_' . $order_status_name] = array(
      'label' => t('To !title', array('!title' => $order_status['title'])),
      'target' => $order_status['name'],
      'origin' => array_keys($order_statuses),
    );
  }

  $events['payment_status']['to_initialized'] = array(
    'label' => t('To Initialized'),
    'target' => 'initialized',
    'origin' => 'unpaid',
  );
  $events['payment_status']['to_completed'] = array(
    'label' => t('To Completed'),
    'target' => 'completed',
    'origin' => 'initialized',
  );
  $events['payment_status']['to_validated'] = array(
    'label' => t('To Validated'),
    'target' => 'validated',
    'origin' => 'completed',
  );
  $events['payment_status']['to_refused'] = array(
    'label' => t('To Refused'),
    'target' => 'refused',
    'origin' => 'initialized',
  );

  return $events;
}

/**
 * Returns an array of events for a specific state type.
 *
 * @param string $type
 *   A state type to return the events for.
 *
 * @return array
 *   An array of events for a specific state type.
 */
function mp_state_machine_events_type($type) {
  $events = mp_state_machine_events();
  return (isset($events[$type])) ? $events[$type] : array();
}

/**
 * Getter callback for the "state" property on node bundles using workflow.
 */
function mp_state_machine_entity_get_state($data, $options, $name, $type, $info) {
  $state_machine = mp_state_machine_load($data);
  return $state_machine->get_current_state();
}

/**
 * Load the state_flow state_machine for the given node.
 *
 * @param $node StdClass
 * @param $reset Boolean
 * @return MPStateFlow
 */
function mp_state_machine_load($entity_type, $entity, $type, $reset = FALSE) {
  $objects = &drupal_static(__FUNCTION__);

  list($entity_id, $vid, $bundle) = entity_extract_ids($entity_type, $entity);

  if (!isset($objects[$entity_type][$entity_id][$type]) || $reset) {
    ctools_include('plugins');

    $plugin = ctools_get_plugins('mp_state_machine', 'plugins', 'mp_state_flow');

    if (!empty($plugin)) {
      $class = ctools_plugin_get_class($plugin, 'handler');
      $state_flow_object = new $class($entity_type, $entity, $type);
      $objects[$entity_type][$entity_id][$type] = $state_flow_object;
    }
  }

  return $objects[$entity_type][$entity_id][$type];
}
