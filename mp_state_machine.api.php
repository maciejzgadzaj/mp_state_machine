<?php

/**
 * @file
 * Hooks provided by the MP State Machine module.
 */

/**
 * Allows modules to alter pre-defined state types.
 *
 * @param array $types
 *   An array of pre-defined state types.
 */
function hook_mp_state_machine_types_alter(&$types) {
  $types['fulfillment_state'] = array(
    'label' => t('Fulfillment state'),
    'description' => t('The current state of the checkout process.'),
    'entity_types' => array('commerce_order'),
  );
}

/**
 * Allows modules to alter pre-defines states.
 *
 * @param array $states
 *   An array of pre-defined states.
 */
function hook_mp_state_machine_states_alter(&$states) {
  $states['fulfillment'] = array(
    'unfulfilled' => array(
      'label' => 'Unfulfilled',
    ),
    'partially_fulfilled' => array(
      'label' => 'Partially fulfilled',
    ),
    'fulfilled' => array(
      'label' => 'Fulfilled',
    ),
  );
}

/**
 * Allows modules to alter pre-defines events.
 *
 * @param array $events
 *   An array of pre-defined events.
 */
function hook_mp_state_machine_events_alter(&$events) {
  $events['fulfillment'] = array(
    'to_partially_fulfilled' => array(
      'label' => t('To Partially fulfilled'),
      'target' => 'partially_fulfilled',
      'origin' => array('unfulfilled'),
    ),
    'to_fulfilled' => array(
      'label' => t('To Fulfilled'),
      'target' => 'fulfilled',
      'origin' => array('partially_fulfilled'),
    ),
  );
}

/**
 * Allows modules to respond when an event is fired.
 *
 * @param string $entity_type
 *   An entity type which state has just been updated.
 * @param object $entity
 *   An entity which state has just been updated.
 * @param string $state_type
 *   A state type which has just been updated.
 * @param string $state
 *   A new state value.
 * @param string $event_key
 *   A name of the event that has just been fired.
 * @param string $log
 *   A log message for the event that has just been fired.
 *
 * @see mp_state_machine_rules_event_info()
 */
function hook_mp_state_machine_event_fired($entity_type, $entity, $state_type, $state, $event_key, $log) {
  $entity_info = entity_get_info($entity_type);
  list($entity_id, , ) = entity_extract_ids($entity_type, $entity);
  $state_types = mp_state_machine_types();
  $states = mp_state_machine_states();

  drupal_set_message(t("!entity_type !entity_id's %state_type has just been updated to %state.", array(
    '!entity_type' => $entity_info['label'],
    '!entity_id' => $entity_id,
    '%state_type' => $state_types[$state_type]['label'],
    '%state' => $states[$state_type][$state]['label'],
  )));
}
