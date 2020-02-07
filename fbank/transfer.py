from flask import g, render_template, request

from login import requirelogin
from fbankdb import *
from debug import *
import bank
import traceback

@catch_err
@requirelogin
def transfer():
    warning = None
    try:
        if ('recipient' in request.form):
            if request.form['fcoins'] == "":
                raise ValueError('Error!')

            fcoins = eval(request.form['fcoins'])
            bank.transfer(g.user.person.username,
                          request.form['recipient'], fcoins)
            warning = "Sent %d fcoins" % fcoins
    except (KeyError, ValueError, AttributeError) as e:
        traceback.print_exc()
        warning = "Transfer to %s failed" % request.form['recipient']

    return render_template('transfer.html', warning=warning)
