<?php

/**
 * @file
 * Views integration for MP State Machine.
 */

/**
 * Implements hook_views_data_alter().
 */
function mp_state_machine_views_data_alter(&$data) {
  // Expose state properties for each entity revision to views.
  $entity_type_state_types = mp_state_machine_entity_type_state_types();
  foreach ($entity_type_state_types as $entity_type => $state_type_info) {
    // Only for entity types that support revisions.
    $entity_info = entity_get_info($entity_type);
    if (!empty($entity_info['revision table']) && isset($data[$entity_info['revision table']])) {
      if ($state_types = $entity_type_state_types[$entity_type]) {
        foreach ($state_types as $state_type => $state_type_info) {
          // Expose state property to views.
          $data[$entity_info['revision table']][$state_type] = array(
            'title' => $state_type_info['label'],
            'help' => $state_type_info['description'],
            'field' => array(
              'handler' => 'mp_state_machine_handler_field_state',
            ),
          );
        }
      }
    }
  }
}
