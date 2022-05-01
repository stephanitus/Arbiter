from app import db
from app import Adjacency
relationship = Adjacency(ParentHostname='HOST1', ChildHostname='HOST2')
db.session.add(relationship)
db.session.commit()