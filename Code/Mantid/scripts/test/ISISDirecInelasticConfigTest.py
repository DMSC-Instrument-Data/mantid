import os
os.environ["PATH"] = r"c:/Mantid/Code/builds/br_master/bin/Release;"+os.environ["PATH"]
import unittest
import shutil
import datetime
from mantid import config
from Direct.ISISDirecInelasticConfig import UserProperties,MantidConfigDirectInelastic



class ISISDirectInelasticConfigTest(unittest.TestCase):
    def __init__(self, methodName):
        # This has been copied from ISIS admin script processing user's office data
        # and all this has the format below
        # if format changes, the class have to change too as the class
        # verifies the data are exactly as defined here
        self.instrument= "MERLIN"
        self.cycle     = "CYCLE20151"
        nrbnumber = "1310168"
        self.rbnumber  = "RB" + nrbnumber
        self.start_date= '20150503'
        self.userID = 'tuf666699'
        return super(ISISDirectInelasticConfigTest, self).__init__(methodName)

    def setUp(self):
        # Create user's folder structure in default save directory.
        # the administrative script (not here) builds all this for real in /home
        targetDir = config['defaultsave.directory']
        self.rbdir = os.path.join(targetDir,self.userID,self.rbnumber)
        self.UserScriptRepoDir = os.path.join(targetDir,'UserScripts')
        self.MapMaskDir        = os.path.join(targetDir,'MapMaskDir')
        self.userRootDir = os.path.join(config['defaultsave.directory'],self.userID)
        if not os.path.exists(self.rbdir):
            os.makedirs(self.rbdir)
        if not os.path.exists(self.UserScriptRepoDir):
            os.makedirs(self.UserScriptRepoDir)
        if not os.path.exists(self.MapMaskDir):
            os.makedirs(self.MapMaskDir)
        if not os.path.exists(self.userRootDir):
            os.makedirs(self.userRootDir)

    def tearDown(self):
        # Clean-up user's folder structure
        if os.path.exists(self.rbdir):
            shutil.rmtree(self.rbdir)
        if os.path.exists(self.UserScriptRepoDir):
            shutil.rmtree(self.UserScriptRepoDir)
        if os.path.exists(self.MapMaskDir):
            shutil.rmtree(self.MapMaskDir)
        if os.path.exists(self.userRootDir):
            shutil.rmtree(self.userRootDir)

    def test_UserProperties(self):
        user = UserProperties()

        user.set_user_properties(self.instrument,self.start_date,self.cycle,self.rbdir)

        id = user._recent_dateID
        self.assertEqual(user.instrument[id],'MERLIN')
        self.assertEqual(user.cycle_IDlist[id],('2015','1'))
        self.assertEqual(user.start_dates[id],datetime.date(2015,05,03))
        self.assertEqual(user.rb_dir[id],self.rbdir)

        self.assertRaises(RuntimeError,user.set_user_properties,'SANS2D',self.start_date,self.cycle,self.rbdir)
        self.assertRaises(RuntimeError,user.set_user_properties,'HET','201400000',self.cycle,self.rbdir)
        self.assertRaises(RuntimeError,user.set_user_properties,'HET',self.start_date,'20144',self.rbdir)
        self.assertRaises(RuntimeError,user.set_user_properties,'HET',self.start_date,'CYC20144',self.rbdir)
        self.assertRaises(RuntimeError,user.set_user_properties,'HET',self.start_date,'CYCLE201444',self.rbdir)
        self.assertRaises(RuntimeError,user.set_user_properties,'HET',self.start_date,self.cycle,'missing/folder')

        rbdir = os.path.join(self.userRootDir,'RB1000666')
        if not os.path.exists(rbdir):
            os.makedirs(rbdir)
        user.set_user_properties('MARI','20000112','CYCLE20001',rbdir)
        if os.path.exists(rbdir):
            shutil.rmtree(rbdir)

        self.assertEqual(len(user.instrument),2)
        self.assertEqual(user._recent_dateID,id)
        self.assertEqual(user.start_dates['2000-01-12'],datetime.date(2000,01,12))

        targetDir = config['defaultsave.directory']
        rbdir = os.path.join(self.userRootDir,'RB1999666')
        if not os.path.exists(rbdir):
            os.makedirs(rbdir)
        user.set_user_properties('MERLIN','20161201','CYCLE20163',rbdir)
        if os.path.exists(rbdir):
            shutil.rmtree(rbdir)

        self.assertEqual(len(user.instrument),3)
        id = user._recent_dateID
        self.assertEqual(id,'2016-12-01')
        self.assertEqual(user.instrument[id],'MERLIN')

    def test_build_config(self):
        # script verifies the presence of a folder, not its contents.
        # for the script to work, let's run it on default save directory
        MantidDir = os.path.split(os.path.realpath(__file__))[0]
        HomeRootDir = config['defaultsave.directory']
        mcf = MantidConfigDirectInelastic(MantidDir,HomeRootDir,self.UserScriptRepoDir,self.MapMaskDir)

        self.assertRaises(RuntimeError,MantidConfigDirectInelastic,'MissingMantidFolder',HomeRootDir,self.UserScriptRepoDir,self.MapMaskDir)
        self.assertRaises(RuntimeError,MantidConfigDirectInelastic,MantidDir,'MissingUserRootFolder',self.UserScriptRepoDir,self.MapMaskDir)
        self.assertRaises(RuntimeError,MantidConfigDirectInelastic,MantidDir,HomeRootDir,'MissingUserScriptRepoDir',self.MapMaskDir)
        self.assertRaises(RuntimeError,MantidConfigDirectInelastic,MantidDir,HomeRootDir,self.UserScriptRepoDir,'MissingMapMaskDir')

        user = UserProperties()
        user.set_user_properties(self.instrument,self.start_date,self.cycle,self.rbdir)

        # clear up the previous
        if os.path.exists(os.path.join(self.userRootDir,'.mantid')):
            shutil.rmtree(os.path.join(self.userRootDir,'.mantid'))


        mcf.init_user(self.userID,user)
        mcf.generate_config()
        self.assertTrue(os.path.exists(os.path.join(self.userRootDir,'.mantid')))
        self.assertTrue(os.path.exists(os.path.join(self.userRootDir,'.mantid','Mantid.user.properties')))

        # clear up
        if os.path.exists(os.path.join(self.userRootDir,'.mantid')):
            shutil.rmtree(os.path.join(self.userRootDir,'.mantid'))





if __name__=="__main__":
    unittest.main()
