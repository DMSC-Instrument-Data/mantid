from __future__ import (absolute_import, division, print_function)

import mantid

from isis_powder.abstract_inst import AbstractInst
from isis_powder.routines.instrument_settings import InstrumentSettings
from isis_powder.routines.param_map_entry import ParamMapEntry
from isis_powder.routines import run_details

import os
import tempfile
import unittest
import warnings


class ISISPowderAbstractInstrumentTest(unittest.TestCase):

    _folders_to_remove = set()
    CALIB_FILE_NAME = "ISISPowderRunDetailsTest.yaml"
    GROUPING_FILE_NAME = "hrpd_new_072_01.cal"

    def _create_temp_dir(self):
        temp_dir = tempfile.mkdtemp()
        self._folders_to_remove.add(temp_dir)
        return temp_dir

    def _find_file_or_die(self, name):
        full_path = mantid.api.FileFinder.getFullPath(name)
        if not full_path:
            self.fail("Could not find file \"{}\"".format(name))
        return full_path

    def _setup_mock_inst(self, calibration_dir, output_dir, suffix=None):
        calib_file_path = self._find_file_or_die(self.CALIB_FILE_NAME)
        grouping_file_path = self._find_file_or_die(self.GROUPING_FILE_NAME)

        return _MockInst(group_file=grouping_file_path, cal_dir=calibration_dir,
                         out_dir=output_dir, cal_map=calib_file_path, suffix=suffix)

    def tearDown(self):
        for folder in self._folders_to_remove:
            try:
                os.rmdir(folder)
            except OSError as exc:
                warnings.warn("Could not remove folder at \"{}\"\n"
                              "Error message:\n{}".format(folder, exc))

    def test_generate_out_file_paths(self):
        cal_dir = self._create_temp_dir()
        out_dir = self._create_temp_dir()

        mock_inst = self._setup_mock_inst(suffix="-suf", calibration_dir=cal_dir, output_dir=out_dir)
        run_number = 15
        run_details_obj = run_details.create_run_details_object(run_number_string=run_number,
                                                                inst_settings=mock_inst._inst_settings,
                                                                is_vanadium_run=False)
        output_paths = mock_inst._generate_out_file_paths(run_details=run_details_obj)

        expected_nxs_filename = os.path.join(out_dir,
                                             "16_4",
                                             "ISISPowderAbstractInstrumentTest",
                                             "MOCK15-suf.nxs")
        self.assertEquals(output_paths["nxs_filename"], expected_nxs_filename)

    def test_set_valid_beam_parameters(self):
        # Setup basic instrument mock
        cal_dir = self._create_temp_dir()
        out_dir = self._create_temp_dir()
        mock_inst = self._setup_mock_inst(calibration_dir=cal_dir, output_dir=out_dir)

        # Test valid parameters are retained
        mock_inst.set_beam_parameters(height=1.234, width=2)
        self.assertEqual(mock_inst._beam_parameters['height'], 1.234)
        self.assertEqual(mock_inst._beam_parameters['width'], 2)

    def test_set_invalid_beam_parameters(self):
        # Setup basic instrument mock
        cal_dir = self._create_temp_dir()
        out_dir = self._create_temp_dir()
        mock_inst = self._setup_mock_inst(calibration_dir=cal_dir, output_dir=out_dir)

        # Test combination of positive / negative raise exceptions
        self.assertRaises(ValueError, mock_inst.set_beam_parameters, height=1.234, width=-2)
        self.assertRaises(ValueError, mock_inst.set_beam_parameters, height=-1.234, width=2)
        self.assertRaises(ValueError, mock_inst.set_beam_parameters, height=-1.234, width=-2)

        # Test non-numerical input
        self.assertRaises(ValueError, mock_inst.set_beam_parameters, height='height', width=-2)
        self.assertRaises(ValueError, mock_inst.set_beam_parameters, height=-1.234, width=True)


class _MockInst(AbstractInst):

    _param_map = [
        ParamMapEntry(ext_name="cal_dir",    int_name="calibration_dir"),
        ParamMapEntry(ext_name="cal_map",    int_name="cal_mapping_path"),
        ParamMapEntry(ext_name="file_ext",   int_name="file_extension", optional=True),
        ParamMapEntry(ext_name="group_file", int_name="grouping_file_name"),
        ParamMapEntry(ext_name="out_dir",    int_name="output_dir"),
        ParamMapEntry(ext_name="suffix",     int_name="suffix", optional=True),
        ParamMapEntry(ext_name="user_name",  int_name="user_name")
    ]
    _advanced_config = {"user_name": "ISISPowderAbstractInstrumentTest"}
    INST_PREFIX = "MOCK"

    def __init__(self, **kwargs):
        self._inst_settings = InstrumentSettings(param_map=self._param_map,
                                                 adv_conf_dict=self._advanced_config,
                                                 kwargs=kwargs)

        super(_MockInst, self).__init__(user_name=self._inst_settings.user_name,
                                        calibration_dir=self._inst_settings.calibration_dir,
                                        output_dir=self._inst_settings.output_dir,
                                        inst_prefix=self.INST_PREFIX)
