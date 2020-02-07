from flask import g, render_template, make_response

import login
from fbankdb import *
from debug import catch_err

@catch_err
def fcoinsjs():
    if login.logged_in():
        return render_template("fcoins.js")
    else:
        return ""
