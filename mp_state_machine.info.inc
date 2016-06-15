<?php
/**
 * @file
 * MP State Machine main module file.
 */

/**
 * Implements hook_entity_property_info_alter().
 *
 * Adds a "state" property on nodes that are configured with state flow.
 */
function mp_state_machine_entity_property_info_alter(&$info) {
  $types = mp_state_machine_types();

  foreach ($types as $type_name => $type_info) {
    foreach ($type_info['entity_types'] as $entity_type) {
      $info[$entity_type]['properties'][$type_name] = array(
        'label' => $type_info['label'],
        'description' => $type_info['description'],
        'type' => 'text',
        'options list' => 'mp_state_machine_entity_property_options_list',
        'getter callback' => 'mp_state_machine_entity_property_get_state',
//        'setter callback' => 'mp_state_machine_entity_property_set_state',
        'entity views field' => TRUE,
      );
    }
  }
}

/**
 * Returns a list of possible values for a specific entity state property.
 *
 * @see mp_state_machine_entity_property_info_alter()
 */
function mp_state_machine_entity_property_options_list($state_type) {
  $options_list = array();

  if ($states = mp_state_machine_states_type($state_type)) {
    foreach ($states as $state_name => $state_info) {
      $options_list[$state_name] = $state_info['label'];
    }
  }

  return $options_list;
}

/**
 * Getter callback for the entity state properties.
 *
 * @see mp_state_machine_entity_property_info_alter()
 */
function mp_state_machine_entity_property_get_state($entity, $options, $state_type, $entity_type, $info) {
  $state_machine = mp_state_machine_load($entity_type, $entity, $state_type);
  return $state_machine->get_current_state();
}

/**
 * Setter callback for the entity state properties.
 *
 * @see mp_state_machine_entity_property_info_alter()
 */
function mp_state_machine_entity_property_set_state(&$entity, $state_type, $value, $langcode, $entity_type, $info) {
  $state_machine = mp_state_machine_load($entity_type, $entity, $state_type);
  $event_name = 'to_' . $value;
  $state_machine->fire_event($event_name);
}
