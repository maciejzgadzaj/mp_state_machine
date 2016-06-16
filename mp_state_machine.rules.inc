<?php

/**
 * @file
 * Rules integration for mp_state_machine module.
 */

/**
 * Implements hook_rules_event_info().
 *
 * Define Rules events that are triggered by the state machine.
 */
function mp_state_machine_rules_event_info() {
  $items = array();

  if ($entity_type_state_types = mp_state_machine_entity_type_state_types()) {
    foreach (array_keys($entity_type_state_types) as $entity_type) {
      $entity_info = entity_get_info($entity_type);
      $items['mp_state_machine_event_fired_' . $entity_type] = array(
        'label' => t('After !entity_type state transition occurs', array('!entity_type' => $entity_info['label'])),
        'group' => t('MP State Machine'),
        'variables' => entity_rules_events_variables($entity_type, $entity_info['label']) + array(
          'entity_type' => array('type' => 'text', 'label' => t('entity type')),
          'state_type' => array('type' => 'text', 'label' => t('state type')),
          'state' => array('type' => 'text', 'label' => t('state')),
          'event_key' => array('type' => 'text', 'label' => t('event key')),
        ),
      );
    }
  }

  return $items;
}
