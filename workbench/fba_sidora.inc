<?php

define("FBA_URI", 'http://fba.org.uk/schemas/fba/fba_rdf/1.0.0/');
define("FOAF_URI", 'http://xmlns.com/foaf/spec/'); //FBA MODIFICATION BY NBYWELL 05/10/2015
define("PURL_TERMS_URI", 'http://purl.org/dc/terms/'); //FBA MODIFICATION BY NBYWELL 05/10/2015
define("PURL_ELEMENTS_URI", 'http://purl.org/dc/elements/1.1/'); //FBA MODIFICATION BY NBYWELL 05/10/2015
define("FBA_MOLES_URI", 'http://fba.org.uk/schemas/moles/3.4.0'); //JIRA AEDA-22

/**
 * Implements hook_user().
 *
 * @TODO add form elements for further info so we can populate more of the eac-cpf record (op == 'register' or op == 'edit')
 * @TODO will also have to update the eac-cpf on user update (op== 'update').
 */
function sidora_user_insert(&$edit, &$account, $category = NULL) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_item');
  module_load_include('inc', 'content_model_viewer', 'Collection');
  module_load_include('inc', 'sidora', 'includes/utils');
  $module_path = drupal_get_path('module', 'sidora');

  $object = sidora_create_user_object($account);

  //create user's private collection object
  $accountpid = $object->id;
  $edit['islandora_user_pid'] = $accountpid;
  //create the namespace from the user name fba-user:100 becomes fbauser100
  $usercollectionns = str_replace(array('-', ':'), '', $accountpid);
  $connection = islandora_get_tuque_connection();
  $repository = $connection->repository;
  $user_collection_object = $repository->constructObject($usercollectionns . ':collection'); // create an object with the given PID
  $user_collection_object->models = array('islandora:collectionCModel');
  $user_collection_object->label = $accountpid . ' Private Collection';
  $user_collection_object->owner = $accountpid;
  $prefix = sidora_get_system_prefix();

  $user_collection_object->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $prefix . ':root');

  sidora_update_datastream($user_collection_object, $module_path . '/fba/collection_policies/fba_sidora_collection_policy.xml', 'COLLECTION_POLICY', NULL, 'text/xml');
  sidora_update_datastream($user_collection_object, sidora_create_default_xacml($account), 'POLICY', NULL, 'text/xml');
  //SFOX add a CHILD_SECURITY XACML, so that objects in the collection can be viewed by owner only.
  sidora_update_datastream($user_collection_object, sidora_create_default_xacml($account), 'CHILD_SECURITY', NULL, 'text/xml');

  $new_fedora_object = islandora_add_object($user_collection_object); //throw an error if this fails?
  //SFOX Add FBA relext of "isOwnerOf"
  $user_account = $repository->getObject($accountpid);
  sidora_set_relationship($user_account->relationships, FBA_URI, 'isOwnerOf', $usercollectionns . ':collection');
}

function sidora_return_fba_concept_rdfs() {
  $contypes = sidora_return_fba_concepts();
  foreach ($contypes as $contype) {
    $retarray[] = $contype[1];
  }
  return $retarray;
}

function sidora_return_fba_resource_rdfs() {
  $returnarray = array_merge(sidora_return_fba_core_resource_rdfs(), sidora_return_fba_supp_resource_rdfs());
  return array_unique($returnarray);
}

function sidora_return_fba_core_resource_rdfs() {
  $restypes = sidora_return_fba_resources();
  foreach ($restypes as $restype) {
    $retarray[] = $restype[1];
  }
  return array_unique($retarray);
}

function sidora_return_fba_supp_resource_rdfs() {
  $supptypes = sidora_return_fba_supplementary();
  foreach ($supptypes as $supptype) {
    $retarray[] = $supptype[1];
  }
  return array_unique($retarray);
}

function sidora_return_fba_concept_cmodels() {
  $concepts = sidora_return_fba_concepts();
  foreach ($concepts as $concept) {
    $retarray[] = $concept[0];
  }
  return $retarray;
}

function sidora_return_fba_resource_cmodels() {
  $resources = sidora_return_fba_resources();
  foreach ($resources as $resource) {
    $retarray[] = $resource[0];
  }
  return $retarray;
}

function sidora_return_fba_concepts() {
  return array(
    array('fba:activity_cmodel', 'hasActivity', 'dtcac', 'Activity'),
    array('fba:dataset_cmodel', 'hasDataset', 'dtcds', 'Dataset'),
  );
}

function sidora_return_fba_resources() {
  return array_merge(sidora_return_fba_data_components(), sidora_return_fba_supplementary());
}

function sidora_return_fba_data_components() {
  return array(
    array('fba:measurement_cmodel', 'hasMeasurement', 'dtcmt', 'Measurement'),
    array('fba:analysis_cmodel', 'hasAnalysis', 'dtcan', 'Analysis'),
    array('fba:litreview_cmodel', 'hasLiteratureReview', 'dtclr', 'Literature Review'),
    array('fba:simulation_cmodel', 'hasSimulation', 'dtcsi', 'Simulation'),
    array('fba:synthesis_cmodel', 'hasSynthesis', 'dtcsy', 'Synthesis'),
  );
}

function sidora_return_fba_supplementary() {
  return array(
    array('fba:supp_basic_image_cmodel', 'hasSupplementaryFile', 'dtcbim', 'Image'),
    array('fba:supp_large_image_cmodel', 'hasSupplementaryFile', 'dtclim', 'Large Image'),
    array('fba:supp_pdf_cmodel', 'hasSupplementaryFile', 'dtcpdf', 'PDF'),
    array('fba:supp_text_cmodel', 'hasSupplementaryFile', 'dtctxt', 'Text'),
    array('fba:supp_excel_cmodel', 'hasSupplementaryFile', 'dtcxl', 'Excel'),
    array('fba:supp_word_cmodel', 'hasSupplementaryFile', 'dtcwd', 'Word'),
    array('si:imageCModel', 'hasSupplementaryFile', 'sibim', 'SI Image'),
    array('si:generalImageCModel', 'hasSupplementaryFile', 'sigim', 'SI Gen Image'),
  );
}

function sidora_return_all_types() {
//array 0 is content model pid
//array 1 is rels ext
//array 2 is namespace
//array 3 is text title
  return array_merge(sidora_return_fba_concepts(), sidora_return_fba_resources(), sidora_return_fba_supplementary());
}

function sidora_get_cmodel_rels_pair() {
  $allcmodeldata = sidora_return_all_types();
  foreach ($allcmodeldata as $cmodeldatarow) {
    $retarray[] = array($cmodeldatarow[0], $cmodeldatarow[1]);
  }
  return $retarray;
}

function sidora_get_cmodel_namespace_pair() {
  $allcmodeldata = sidora_return_all_types();
  foreach ($allcmodeldata as $cmodeldatarow) {
    $retarray[] = array($cmodeldatarow[0], $cmodeldatarow[2]);
  }
  return $retarray;
}

function sidora_get_namespace_rel_pair() {
  $allcmodeldata = sidora_return_all_types();
  foreach ($allcmodeldata as $cmodeldatarow) {
    $retarray[] = array($cmodeldatarow[2], $cmodeldatarow[1]);
  }
  return $retarray;
}

function sidora_get_rel_from_pid($pid) {
  $pidprefix = substr($pid, 0, strpos($pid, ':'));
  $pairs = sidora_get_namespace_rel_pair();
  foreach ($pairs as $pair) {
    if (strcmp($pair[0], $pidprefix) === 0) {
      $resourcerdf = $pair[1];
      break;
    }
  }
  return $resourcerdf;
}

function sidora_get_cmodel_from_pid($pid) {
  $pidprefix = substr($pid, 0, strpos($pid, ':'));
  $prefix = sidora_get_system_prefix();
  if (strcmp($pidprefix, $prefix . '-user') === 0) {
    $resourcerdf = 'fba:peopleCModel'; //SFOX Nasty hack, change this
  }
  else {
    $pairs = sidora_get_cmodel_namespace_pair();
    foreach ($pairs as $pair) {
      if (strcmp($pair[1], $pidprefix) === 0) {
        $resourcerdf = $pair[0];
        break;
      }
    }
  }
  return $resourcerdf;
}

function sidora_fba_return_vocab_cmodels() {
  return array(
    'fba:vocabpref_CModel',
    'fba:vocabnonpref_CModel',
    'fba:geopref_CModel',
    'fba:geononpref_CModel',
  );
}

function sidora_fba_supp_media_submit(array $form, array &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_supp_media_validate($form, &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_supp_gen_submit(array $form, array &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_supp_gen_validate($form, &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_simulation_submit(array $form, array &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_simulation_validate($form, &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_synthesis_submit(array $form, array &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_synthesis_validate($form, &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
// Check from date.
  $from = explode('/', $form_state['values']['date_filter']['date_filter_from']);
  $to = explode('/', $form_state['values']['date_filter']['date_filter_to']);
  if (!checkdate(intval($from[1]), intval($from[2]), intval($from[0]))) {
//Can't use form_set_error as it's all javascript now
    form_set_error($form_key . '][date_filter_from', t('<em>From</em> date is not formatted correctly.'));
  }
}

function sidora_fba_measurement_submit(array $form, array &$form_state) {
  $x = 1;
//$form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_measurement_validate($form, &$form_state) {
  $x = 1;
//$form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_dataset_submit(array $form, array &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_dataset_validate($form, &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_litreview_submit(array $form, array &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_litreview_validate($form, &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_analysis_submit(array $form, array &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_analysis_validate($form, &$form_state) {
  $form_key = $form_state['triggering_element']['#form_key'];
}

function sidora_fba_activity_submit(array $form, array &$form_state) {
  $x = 1;
//foreach ($form_state['groups'] as $group) {
//  $status = skinr_context_group_import($group, TRUE) && $status;
// }
//drupal_set_message(t('The skin configuration groups have been saved.'));
//if (!$status) {
//    drupal_set_message(t('Not all skin configuration groups could be saved!'), 'error', FALSE);
//  }
}

function sidora_fba_activity_validate(&$form, &$form_state) {
//$error_message = t('Abstract is empty!');
//if (empty($form_state['values']['abstract'])) {
// Error.
//    form_error($form['abstract'], $error_message);
//    return;
//}
}

function sidora_fba_vocabpref_submit(&$form, &$form_state) {
  
}

/**
 * Implements hook_xml_islandora_content_model_forms_get_transforms of the XML Form Builder module
 * This allows admins to pick these transforms when using the XML Form Builder
 */
function sidora_islandora_content_model_forms_get_transforms() {
  $p = drupal_get_path('module', 'sidora');
  $transforms = array();
  $include_path = "$p/transforms";
  $dir = opendir($include_path);
  if ($dir !== FALSE) {
    while (($file = readdir($dir)) !== FALSE) {
      if (preg_match('/\.xslt?$/', $file)) {
        $transforms["$include_path/$file"] = $file . " (sidora)";
        $transforms[$file] = $file . " (sidora; legacy location)";
      }
    }
  }
  closedir($dir);
  return $transforms;
}

function sidora_ingest_form_submit(array $form, array &$form_state) {
  $x = 1;
}

function sidora_form_submit(array $form, array &$form_state) {
  $x = 1;
}

function system_is_fba() {
  if (strcmp(sidora_get_system_prefix(), FBA_PREFIX) === 0) {
    $is_fba = TRUE;
  }
  else {
    $is_fba = FALSE;
  }
  return $is_fba;
}

function sidora_form_validate($form, &$form_state) {
  module_load_include('inc', 'fedora_repository', 'api/fedora_collection');
  $pid = $form_state['values']['pid'];

  $objectHelper = new ObjectHelper();
  $contentModels = $objectHelper->get_content_models_list($pid);
  foreach ($contentModels as $contentModel) {
    if ($contentModel->pid == 'islandora:collectionCModel') {
      $member_pids = get_related_items_as_array($pid, 'isMemberOfCollection');
      if (is_array($member_pids) && !empty($member_pids)) {
        form_set_error('new_collection_pid', t("Please purge all members of this collection before deleting the collection itself."));
        return;
      }
    }
  }
}

function sidora_return_fba_extensions() {
  $file_extensions = array(
    'fba:activity_cmodel' => array('csv'),
    'fba:dataset_cmodel' => array('csv'),
    'fba:measurement_cmodel' => array('csv'),
    'fba:analysis_cmodel' => array('csv'),
    'fba:litreview_cmodel' => array('csv'),
    'fba:simulation_cmodel' => array('csv'),
    'fba:synthesis_cmodel' => array('csv'),
    'fba:supp_basic_image_cmodel' => array('jpg', 'jpeg', 'png', 'gif'),
    'fba:supp_pdf_cmodel' => array('pdf'),
    'fba:supp_text_cmodel' => array('txt'),
    'fba:supp_excel_cmodel' => array('xls', 'xlsx'),
    'fba:supp_word_cmodel' => array('doc', 'docx'),
    '*' => array('*'),
  );
  return $file_extensions;
}

function sidora_return_fba_resource_types() {
  $rescmodels = sidora_return_fba_resource_cmodels();
  $sidoramodels = array('si:imageCModel', 'si:generalImageCModel',);
  $resource_types = array(
    'all' => array_merge($rescmodels, $sidoramodels),
    'images' => array(
      'si:imageCModel', 'si:generalImageCModel',
      'fba:supp_basic_image_cmodel',),
    'pdf' => array(
      'fba:supp_pdf_cmodel',),
    'csv' => array(
      'fba:analysis_cmodel', 'fba:litreview_cmodel', 'fba:simulation_cmodel', 'fba:synthesis_cmodel',),
    'audio' => array('islandora:sp-audioCModel'),
    'video' => array('islandora:sp_videoCModel'),
    'text' => array('fba:supp_text_cmodel',),
  ); //SFOX need to add these 'fba:supp_word_cmodel','fba:supp_excel_cmodel',),
  return $resource_types;
}

function sidora_return_fba_ontology_defaults() {
  $returnvalue = '{
    "FBA Activity": {
        "description": "FBA Activity",
        "children": {},
        "model": "fba:activity_cmodel",
        "form": "FBA Activity",
        "ontology-id": "FBAActivityOntologyID"
    },
    "FBA Dataset": {
        "description": "FBA Dataset",
        "children": {},
        "model": "fba:dataset_cmodel",
        "form": "FBA Dataset", 
        "ontology-id": "FBADatasetOntologyID"
  }
  }';
  return $returnvalue;
}

function sidora_return_fba_resource_ontology_json_default() {
  $returnvalue = '{
        "FBA Synthesis": {
            "description": "This resource is an FBA Synthesis.",
            "children": {},
            "model": "fba:synthesis_cmodel",
            "form": "FBA Synthesis",
            "ontology-id": "FBASynthesisOntologyID"
        },
        "FBA Simulation": {
            "description": "This resource is an FBA Simulation.",
            "children": {},
            "model": "fba:simulation_cmodel",
            "form": "FBA Simulation",
            "ontology-id": "FBASimulationOntologyID"
        },
        "FBA Literature Review": {
            "description": "This resource is an FBA Literature Review.",
            "children": {},
            "model": "fba:litreview_cmodel",
            "form": "FBA Literature Review",
            "ontology-id": "FBALiteratureReviewOntologyID"
        },
        "FBA Measurement": {
            "description": "This resource is an FBA Measurement.",
            "children": {},
            "model": "fba:measurement_cmodel",
            "form": "FBA Measurement",
            "ontology-id": "FBAMeasurementOntologyID"
        },
        "FBA Analysis": {
            "description": "This resource is an FBA Analysis.",
            "children": {},
            "model": "fba:analysis_cmodel",
            "form": "FBA Analysis",
            "ontology-id": "FBAAnalysisOntologyID"
        },
        "FBA Supplementary Media": {
            "description": "This resource is an FBA Supplementary Media.",
            "children": {},
            "model": "fba:supp_basic_image_cmodel",
            "form": "FBA Supplementary Media",
            "ontology-id": "FBASupplementaryMediaOntologyID"
        },
        "FBA Supplementary Generic Text": {
            "description": "This resource is an FBA Supplementary Generic Text.",
            "children": {},
            "model": "fba:supp_text_cmodel",
            "form": "FBA Supplementary Generic Text",
            "ontology-id": "FBASupplementaryGenericTextOntologyID"
        },
        "FBA Supplementary Generic Word": {
            "description": "This resource is an FBA Supplementary Generic Word.",
            "children": {},
            "model": "fba:supp_text_cmodel",
            "form": "FBA Supplementary Generic Word",
            "ontology-id": "FBASupplementaryGenericWordOntologyID"
        },
        "FBA Supplementary Generic Excel": {
            "description": "This resource is an FBA Supplementary Generic Excel.",
            "children": {},
            "model": "fba:supp_text_cmodel",
            "form": "FBA Supplementary Generic Excel",
            "ontology-id": "FBASupplementaryGenericExcelOntologyID"
        },
        "FBA Supplementary Generic PDF": {
            "description": "This resource is an FBA Supplementary Generic PDF.",
            "children": {},
            "model": "fba:supp_text_cmodel",
            "form": "FBA Supplementary Generic PDF",
            "ontology-id": "FBASupplementaryGenericPDFOntologyID"
        },
        "Tabular data": {
        "description": "This resource is a Tabular data file.",
        "children": {},
        "model": "si:datasetCModel",
        "form": "Sidora Tabular File Form"
    },    
    "General Image": {
    "description": "This resource is a basic image.",
    "children": {},
    "model": "si:generalImageCModel",
    "form": "Sidora Basic Image Form"
    },
    "Image": {
    "description": "This resource is an image.",
    "children": {},
    "model": "si:imageCModel",
    "form": "FBA Supplementary Media"
    }}';
  return $returnvalue;
}

function sidora_fba_pre_submitter($form, &$form_state) {
  if (array_key_exists('association', $form_state)) {
    $association = sidora_find_association($form_state);
    $contentmodelpid = $association['content_model'];
  }
  else {
    //This route is when adding a new term through eg the data.local site
    $contentmodelpid = $form_state['build_info']['args'][0]['models'][0];
  }

  $cmodels = sidora_fba_return_vocab_cmodels();
  if (in_array($contentmodelpid, $cmodels)) {
    if ($form_state['submitted']) {
      $pid = $form_state['build_info']['args'][0]['objects'][0]->id;
      $button = $form_state['clicked_button']['#name']; //ingest or update
      switch ($button) {
        case 'ingest': {
            module_load_include('inc', 'fba_object_linker', 'objectlinker/ObjectLinker');
            $relations = preprocess_links($form, $form_state, false, $pid);         // object then subject relations to be created.

            break;
          }
        case 'update': {
            module_load_include('inc', 'fba_object_linker', 'objectlinker/EditObjectMetadataForm');
            $edit_form = new EditObjectMetadataForm();
            $edit_form->submit($form, $form_state, $label_field);
            break;
          }
      }
    }
  }
}

function sidora_fba_submitter($form, &$form_state) {
  if (array_key_exists('association', $form_state)) {
    $association = sidora_find_association($form_state);
    $contentmodelpid = $association['content_model'];
  }
  else {
    //This route is when adding a new term through eg the data.local site
    $contentmodelpid = $form_state['build_info']['args'][0]['models'][0];
  }

  //Object Linker for resources only.
  $resourcecmodels = sidora_return_fba_resource_cmodels();

  if (in_array($contentmodelpid, $resourcecmodels)) {

    if ($form_state['submitted']) {
      $pid = $form_state['pid'];
      $button = $form_state['clicked_button']['#name']; //ingest or update
      switch ($button) {
        case 'ingest': {
            module_load_include('inc', 'fba_object_linker', 'objectlinker/ObjectLinker');
            $ocrProcessingActivated = false; // This variable will not longer be required once the processing has been fully tested.
            $pdfFilePath = '';
            $escapedPdfFile = '';
            $escapedPdfDirPath = '';
            $tmpDir = '';
            $tmpDirPath = '';
            $concatOutputFilePath = '';
            $ocrMsg = '';
            $ocrProcessingRequired = true;
            /*
             * Create Object-Object links.
             */
            process_links($form, $form_state, false, $pid);
//        }
            // The ocrProcessingActivated condition has been included below
            // to disable this functionality until it has been fully tested.
            if ($ocrProcessingRequired && $ocrProcessingActivated) {
              /*
               * Extract the text from the PDF in the temporary directory.
               * directory, add it as an OCR datastream and then
               * remove the temporary directory.
               */

              extractOcrFromPdf($pid, $pdfFilePath, $escapedPdfFile, $escapedPdfDirPath, $tmpDir, $tmpDirPath, $concatOutputFilePath, $ocrMsg);
            }
            $_SESSION['fedora_ingest_files'] = '';
            $form_state['storage'] = NULL;
            break;
          }
        case 'update': {
            module_load_include('inc', 'fba_object_linker', 'objectlinker/EditObjectMetadataForm');
            $edit_form = new EditObjectMetadataForm();
            $edit_form->submit($form, $form_state, $label_field);
            break;
          }
      }
    }
  }

  switch ($contentmodelpid) {
    case 'fba:dataset_cmodel': {
        sidora_fba_dataset_submit($form, $form_state);
        break;
      }
    case 'fba:activity_cmodel': {
        sidora_fba_activity_submit($form, $form_state);
        break;
      }
    case 'fba:analysis_cmodel': {
        resource_form_submit($form, $form_state);
        break;
      }
    case 'fba:measurement_cmodel': {
        sidora_fba_measurement_submit($form, $form_state);
        break;
      }
    case 'fba:litreview_cmodel': {
        sidora_fba_litreview_submit($form, $form_state);
        break;
      }
    case 'fba:simulation_cmodel': {
        sidora_fba_simulation_submit($form, $form_state);
        break;
      }
    case 'fba:synthesis_cmodel': {
        sidora_fba_synthesis_submit($form, $form_state);
        break;
      }
    case 'fba:supp_basic_image_cmodel': {
        sidora_fba_supp_media_submit($form, $form_state);
        break;
      }
    //case 'fba:supp_large_image_cmodel':
    case 'fba:supp_pdf_cmodel': {
        sidora_fba_supp_gen_submit($form, $form_state);
        break;
      }
    case 'fba:supp_text_cmodel': {
        sidora_fba_supp_gen_submit($form, $form_state);
        break;
      }
    case 'fba:supp_excel_cmodel': {
        sidora_fba_supp_gen_submit($form, $form_state);
        break;
      }
    case 'fba:supp_word_cmodel': {
        sidora_fba_supp_gen_submit($form, $form_state);
        break;
      }
    case 'fba:vocabpref_CModel': {
        sidora_fba_vocabpref_submit($form, $form_state);
        break;
      }
    default: {
        
      }
  }
}

function sidora_fba_validater($form, &$form_state) {
  module_load_include('inc', 'fba_object_linker', 'objectlinker/ObjectLinker');

  if (array_key_exists('association', $form_state)) {
    $association = sidora_find_association($form_state);
    $contentmodelpid = $association['content_model'];
  }
  else {
    //This route is when adding a new term through eg the data.local site
    $contentmodelpid = $form_state['build_info']['args'][0]['models'][0];
  }

  //Place our presubmit handler just before islandora's.
  $presubmitpos = array_search('islandora_ingest_form_submit', $form_state['submit_handlers']);
  array_splice($form_state['submit_handlers'], $presubmitpos, 0, 'sidora_fba_pre_submitter');
  $form_state['submit_handlers'][] = 'sidora_fba_submitter';

  //Object Linker for resources only.
  $resourcecmodels = sidora_return_fba_resource_cmodels();

  if (in_array($contentmodelpid, $resourcecmodels)) {
    //change this to $_SESSION['file_uploads'] or remove as files have to be uploaded before submission can procede?
    if (isset($_FILES['files']['name']['file_upload']) && $_FILES['files']['name']['file_upload'] == 'text/xml') {
      $tmpDir = file_directory_path();
      if (!$file = file_save_upload('file_upload', NULL, $tmpDir)) {
        drupal_set_message('No file loaded', 'warning');
        return;
      }
      $file = "$tmpDir/{$file->filename}";
      $mods_xml = file_get_contents($file);
      $document = DOMDocument::loadXml($mods_xml);
      file_delete($file);
      if (!$document) {
        drupal_set_message('This does not appear to be a valid XML file', 'warning');
        return;
      }
    }
    else {
      $xml_form = new XMLForm($form_state);
      $doc = $xml_form->submit($form, $form_state);
      $document = $doc->document;
    }

    //$contentmodelpid = sidora_get_cmodel_from_pid($pid);
    $collection_pid = drupal_static('parent_collection_pid');

    $content_model_dsid = 'ISLANDORACM'; //Should be ISLANDORACM from collection policy maybe?
    $dsid = 'DTCA';
    //SFOX Get the form name, 
    $form_name = get_object_linker_form_name('', $contentmodelpid);
    /*    $existing_json_sidora_form_config = variable_get('xsl_form_config', '');
      $sfc = json_decode($existing_json_sidora_form_config);

      foreach ($sfc->xsl_config as $config_line) {
      if (strcmp($contentmodelpid, $config_line->model) === 0) {
      $form_name = $config_line->form;
      break;
      }
      } */
    //$should_redirect = TRUE;

    $relationship = NULL;
    //$pid = NULL;
    module_load_include('inc', 'fba_object_linker', 'objectlinker/IngestObjectMetadataForm');

    $collection_policy = loadFromCollection($collection_pid); //COLLECTION PID is parent pid
    if ($collection_policy !== FALSE) { //Collection policy exists; use it!
      $relationship = $collection_policy->getRelationship();
      $pid = $collection_policy->getNextPid($content_model_dsid, $contentmodelpid);
    }

    if (!empty($pid)) {
      $label = $object->label;
      if (empty($label)) {
        $label = "Undefined";
      }
      if (empty($form['#post']['active-toggle'])) {
        $state = 'A';
      }
      else {
        $state = $form['#post']['active-toggle'] ? 'A' : 'I';
      }

      // added ne functionality to static thumbnails to objects if values are provided in forms
      $module = isset($form_state['values']['module']) ? $form_state['values']['module'] : NULL;
      $image_dir = isset($form_state['values']['image_directory']) ? $form_state['values']['image_directory'] : NULL;
      $thumbnail_file = isset($form_state['values']['thumbnail_file']) ? $form_state['values']['thumbnail_file'] : NULL;

      if ($module && $thumbnail_file) {
        $stored_thumbnail = drupal_get_path('module', $module) . "/$image_dir/$thumbnail_file";
        $stored_thumbnail = preg_replace('/\/\//', '/', $stored_thumbnail);
        file_copy($stored_thumbnail, file_directory_path());
        $thumbnail = file_directory_path() . '/' . $thumbnail_file;
        $_SESSION['fedora_ingest_files']['TN'] = $thumbnail;
      }

      $uploaded_file = isset($form_state['values']['ingest-file-location']) && trim($form_state['values']['ingest-file-location']) !== '';
      $ingest_file_location = $uploaded_file ? $form_state['values']['ingest-file-location'] : NULL;
      if (isset($ingest_file_location)) {
        $ingest_file_location = is_array($ingest_file_location) ? $ingest_file_location : array('OBJ' => $ingest_file_location);
      }
      if (isset($_SESSION['fedora_ingest_files']) && is_array($_SESSION['fedora_ingest_files'])) {
        foreach ($_SESSION['fedora_ingest_files'] as $key => $value) {
          $ingest_file_location[$key] = $value;
        }
      }

      /**
       *
       * FBA MODIFICATION BY NBYWELL on 06/03/2014. Check content of
       * the 'rdf_relationships' variable which triggers the
       * Object Linker functionality.
       *
       */
      module_load_include('inc', 'fba_object_linker', 'ObjectLinker');
      $duplicateEntryMsg = NULL;
      $ocrProcessingRequired = false;
      $ocrProcessingActivated = false; // This variable will not longer be required once the processing has been fully tested.

      /**
       *
       * This is a check to determine if an entry already exists
       * in the current collection.
       */
      switch ($collection_pid) {
        // List of collections in which duplicates are not permitted.
        case "fba:vocabpref_collection":
        case "fba:vocabnonpref_collection":
        case "fba:geopref_collection":
        case "fba:geononpref_collection": {
            $sparqlQuery = 'SELECT ?title ?pid WHERE { ?pid <http://purl.org/dc/elements/1.1/title> ?title; <fedora-model:state> <fedora-model:Active>; <fedora-rels-ext:isMemberOfCollection> <info:fedora/' . $collection_pid . '> FILTER regex(str(?title), "^' . $label . '$", "i") } LIMIT 10';
            $duplicateEntryMsg = check_for_duplicate_label($label, $sparqlQuery);
            break;
          }
      }

      if ($duplicateEntryMsg) {
        drupal_set_message($duplicateEntryMsg);
      }
      else {
        // The ocrProcessingActivated condition has been included below
        // to disable this functionality until it has been fully tested.
        if ((!strcmp($contentmodelpid, 'fba:library_monograph_cmodel') ||
            !strcmp($contentmodelpid, 'fba:library_chapter_cmodel') ||
            !strcmp($contentmodelpid, 'fba:library_article_cmodel')) && $ocrProcessingActivated) {
          /*
           * Create a temporary directory in which to perform the
           * OCR processing and copy the PDF file into it as the
           * orginal is automatically removed as part of the
           * ingestion process.
           */
          module_load_include('inc', 'islandora_content_model_forms', 'OcrProcessor');
          $pdfFilePath = '';
          $escapedPdfFile = '';
          $escapedPdfDirPath = '';
          $tmpDir = '';
          $tmpDirPath = '';
          $ocrMsg = '';
          $ocrProcessingRequired = true;
          preprocess_pdf_file($ingest_file_location, $pdfFilePath, $escapedPdfFile, $escapedPdfDirPath, $tmpDir, $tmpDirPath, $ocrProcessingRequired, $ocrMsg);
        }

        module_load_include('inc', 'fba_object_linker', 'objectlinker/ObjectLinker');
        if (variable_get('rdf_relationships', NULL)) {
          /*
           * Pre-process Object Linking data.
           */
          preprocess_link_data($document);
        }
      }
    }
  }
  $cmodels = sidora_fba_return_vocab_cmodels();
  if (in_array($contentmodelpid, $cmodels)) {
    $xml_form = new XMLForm($form_state);
    $doc = $xml_form->submit($form, $form_state);
    $document = $doc->document;
    preprocess_link_data($document);
  }
  //End object linker

  switch ($contentmodelpid) {
    case 'fba:dataset_cmodel': {
        sidora_fba_dataset_validate($form, $form_state);
        break;
      }
    case 'fba:activity_cmodel': {
        sidora_fba_activity_validate($form, $form_state);
        break;
      }
    case 'fba:analysis_cmodel': {
        sidora_fba_analysis_validate($form, $form_state);
        break;
      }
    case 'fba:measurement_cmodel': {
        sidora_fba_measurement_validate($form, $form_state);
        break;
      }
    case 'fba:litreview_cmodel': {
        sidora_fba_litreview_validate($form, $form_state);
        break;
      }
    case 'fba:simulation_cmodel': {
        sidora_fba_simulation_validate($form, $form_state);
        break;
      }
    case 'fba:synthesis_cmodel': {
        sidora_fba_synthesis_validate($form, $form_state);
        break;
      }
    case 'fba:supp_basic_image_cmodel': {
        sidora_fba_supp_media_validate($form, $form_state);
        break;
      }
    //case 'fba:supp_large_image_cmodel':
    case 'fba:supp_pdf_cmodel': {
        sidora_fba_supp_gen_validate($form, $form_state);
        break;
      }
    case 'fba:supp_text_cmodel': {
        sidora_fba_supp_gen_validate($form, $form_state);
        break;
      }
    case 'fba:supp_excel_cmodel': {
        sidora_fba_supp_gen_validate($form, $form_state);
        break;
      }
    case 'fba:supp_word_cmodel': {
        sidora_fba_supp_gen_validate($form, $form_state);
        break;
      }
    default: {
        
      }
  }
}
