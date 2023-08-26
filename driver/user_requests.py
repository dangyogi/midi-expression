# user_requests.py

r'''

Commands for Driver:
    Controller: M, P, or L
    $h<controller>      -- show I2C requests available for <controller>
'''

import utils


def h(line):
    r'''Help

    $h<controller>      -- show I2C requests available for <controller>
    '''
    #print("help", repr(line))
    controller_letter = line[2].upper()
    if controller_letter == 'D':
        print()
        print("Driver commands:")
        print("$h<controller> -- show I2C requests available for <controller>, can be 'd' for Driver")
        print()
    else:
        controller = utils.Controllers1[controller_letter]
        print()
        print(controller.name, "Controller")
        print("  Requests:")
        if hasattr(controller, 'requests'):
            for request in controller.requests:
                print("   ", request)
        else:
            print("    None")
        print()
        print("  Reports:")
        if hasattr(controller, 'reports'):
            for report in controller.reports:
                print("   ", report)
        else:
            print("    None")
        print()

