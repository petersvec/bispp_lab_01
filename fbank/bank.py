from fbankdb import *
from debug import *

import time

def transfer(sender, recipient, fcoins):
    persondb = person_setup()
    senderp = persondb.query(Person).get(sender)
    recipientp = persondb.query(Person).get(recipient)

    sender_balance = senderp.fcoins - fcoins
    recipient_balance = recipientp.fcoins + fcoins

    if sender_balance < 0 or recipient_balance < 0:
        raise ValueError()

    senderp.fcoins = sender_balance
    recipientp.fcoins = recipient_balance
    persondb.commit()

    transfer = Transfer()
    transfer.sender = sender
    transfer.recipient = recipient
    transfer.amount = fcoins
    transfer.time = time.asctime()

    transferdb = transfer_setup()
    transferdb.add(transfer)
    transferdb.commit()

def balance(username):
    db = person_setup()
    person = db.query(Person).get(username)
    return person.fcoins

def get_log(username):
    db = transfer_setup()
    return db.query(Transfer).filter(or_(Transfer.sender==username,
                                         Transfer.recipient==username))

