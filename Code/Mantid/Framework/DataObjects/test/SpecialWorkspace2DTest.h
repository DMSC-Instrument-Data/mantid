#ifndef MANTID_DATAOBJECTS_SPECIALWORKSPACE2DTEST_H_
#define MANTID_DATAOBJECTS_SPECIALWORKSPACE2DTEST_H_

#include "MantidDataObjects/SpecialWorkspace2D.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid;

class SpecialWorkspace2DTest : public CxxTest::TestSuite
{
public:


  void test_default_constructor()
  {
    SpecialWorkspace2D_sptr ws(new SpecialWorkspace2D());
    TSM_ASSERT_THROWS_ANYTHING("Can't init with > 1 X or Y entries.",  ws->initialize(100, 2, 1));
    TSM_ASSERT_THROWS_ANYTHING("Can't init with > 1 X or Y entries.",  ws->initialize(100, 1, 2));
    TS_ASSERT_THROWS_NOTHING( ws->initialize(100, 1, 1) );

    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 100);
    TS_ASSERT_EQUALS( ws->blocksize(), 1);
  }

  void test_constructor_from_Instrument()
  {
    // Fake instrument with 5*9 pixels with ID starting at 1
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws(new SpecialWorkspace2D(inst));

    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 45);
    TS_ASSERT_EQUALS( ws->blocksize(), 1);
    TS_ASSERT_EQUALS( ws->getInstrument()->getName(), "basic"); // Name of the test instrument
    const std::set<detid_t> & dets = ws->getSpectrum(0)->getDetectorIDs();
    TS_ASSERT_EQUALS(dets.size(), 1);

    TS_ASSERT_EQUALS( ws->getDetectorID(0), 1);
    TS_ASSERT_EQUALS( ws->getDetectorID(1), 2);
  }

  void test_setValue_getValue()
  {
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws(new SpecialWorkspace2D(inst));

    TS_ASSERT_DIFFERS( ws->getValue(1), 12.3 );
    TS_ASSERT_THROWS_NOTHING( ws->setValue(1, 12.3) );
    TS_ASSERT_DELTA( ws->getValue(1), 12.3, 1e-6 );
    TS_ASSERT_THROWS_ANYTHING( ws->setValue(46, 789) );
    TS_ASSERT_THROWS_ANYTHING( ws->setValue(-1, 789) );
    TS_ASSERT_THROWS_ANYTHING( ws->getValue(47) );
    TS_ASSERT_THROWS_ANYTHING( ws->getValue(-34) );
    TS_ASSERT_EQUALS( ws->getValue(47, 5.0), 5.0 );
    TS_ASSERT_EQUALS( ws->getValue(147, -12.0), -12.0 );

  }

  void test_binaryOperator(){
    Instrument_sptr inst1 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws1(new SpecialWorkspace2D(inst1));

    Instrument_sptr inst2 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
    SpecialWorkspace2D_sptr ws2(new SpecialWorkspace2D(inst2));

    // 1. AND operation
    ws1->setValue(2, 1);
    ws2->setValue(2, 1);
    ws1->BinaryOperation(ws2, AND);
    TS_ASSERT_EQUALS( ws1->getValue(2), 2);

    ws1->setValue(2, 0);
    ws2->setValue(2, 1);
    ws1->BinaryOperation(ws2, AND);
    TS_ASSERT_EQUALS( ws1->getValue(2), 0);

    ws1->setValue(2, 1);
    ws2->setValue(2, 0);
    ws1->BinaryOperation(ws2, AND);
    TS_ASSERT_EQUALS( ws1->getValue(2), 0);

    ws1->setValue(2, 0);
    ws2->setValue(2, 0);
    ws1->BinaryOperation(ws2, AND);
    TS_ASSERT_EQUALS( ws1->getValue(2), 0);

    // 2. OR operation
    ws1->setValue(2, 1);
    ws2->setValue(2, 1);
    ws1->BinaryOperation(ws2, OR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 1);

    ws1->setValue(2, 0);
    ws2->setValue(2, 1);
    ws1->BinaryOperation(ws2, OR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 1);

    ws1->setValue(2, 1);
    ws2->setValue(2, 0);
    ws1->BinaryOperation(ws2, OR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 1);

    ws1->setValue(2, 0);
    ws2->setValue(2, 0);
    ws1->BinaryOperation(ws2, OR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 0);

    // 3. XOR operation
    // 2. OR operation
    ws1->setValue(2, 1);
    ws2->setValue(2, 1);
    ws1->BinaryOperation(ws2, XOR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 0);

    ws1->setValue(2, 0);
    ws2->setValue(2, 1);
    ws1->BinaryOperation(ws2, XOR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 1);

    ws1->setValue(2, 1);
    ws2->setValue(2, 0);
    ws1->BinaryOperation(ws2, XOR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 1);

    ws1->setValue(2, 0);
    ws2->setValue(2, 0);
    ws1->BinaryOperation(ws2, XOR);
    TS_ASSERT_EQUALS( ws1->getValue(2), 0);

  }

  void test_checkcompatible(){
     Instrument_sptr inst1 = ComponentCreationHelper::createTestInstrumentCylindrical(5);
     SpecialWorkspace2D_sptr ws1(new SpecialWorkspace2D(inst1));

     Instrument_sptr inst2 = ComponentCreationHelper::createTestInstrumentCylindrical(6);
     SpecialWorkspace2D_sptr ws2(new SpecialWorkspace2D(inst2));

     // 1. AND operation
     ws1->setValue(2, 1);
     ws2->setValue(2, 1);

     TS_ASSERT_THROWS_ANYTHING( ws1->BinaryOperation(ws2, AND) );

  }

};


#endif /* MANTID_DATAOBJECTS_SPECIALWORKSPACE2DTEST_H_ */

