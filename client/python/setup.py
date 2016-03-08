#from distribute_setup import use_setuptools
#use_setuptools()

from setuptools import setup, find_packages
from os.path import dirname, join

here = dirname(__file__)
setup(
    name='ledger-ledgerwalletproxy',
    version='0.1.0',
    author='BTChip',
    author_email='hello@ledger.fr',
    description='Python library to communicate with Ledger Wallet Proxy application (development TEE bridge)',
    packages=find_packages(),
    install_requires=['pyelftools', 'protobuf'],
    extras_require = {
    },
    include_package_data=True,
    zip_safe=False,
    classifiers=[
	'License :: OSI Approved :: Apache Software License',
        'Operating System :: POSIX :: Linux',
        'Operating System :: POSIX :: Windows',
	'Operating System :: MacOS :: MacOS X'
    ]
)

