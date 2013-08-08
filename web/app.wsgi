import glob
import os
import site

os.chdir(os.path.join(os.path.dirname(__file__), 'stage'))
site_dir = os.path.join(os.getcwd(), glob.glob('lib/python*/site-packages')[0])
site.addsitedir(site_dir)

import app.main
application = app.main.application
