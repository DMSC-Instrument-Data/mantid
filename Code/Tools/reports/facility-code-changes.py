from __future__ import (absolute_import, division, print_function, unicode_literals)

__author__ = 'Stuart Campbell'

import datetime
import subprocess
import csv
import argparse
import os

if __name__ == '__main__':
    print("Generating some random metrics...\n")

    lines_removed_as_negative = False

    # data = {}
    facility_commits = {}
    facility_changed = {}
    facility_added = {}
    facility_removed = {}

    parser = argparse.ArgumentParser()
    parser.add_argument('repository', type=str, help='Location of git repo')
    args = parser.parse_args()
    repolocation = args.repository

    if not os.path.isdir(repolocation):
        print("ERROR: Specified repository location is not a directory.")
        exit()

    organisations = ['STFC', 'ORNL', 'ESS', 'ILL', 'PSI', 'ANSTO', 'KITWARE', 'JUELICH', 'OTHERS']

    domains = {}
    domains = {'stfc.ac.uk': 'STFC',
               'clrc.ac.uk': 'STFC',
               'ornl.gov': 'ORNL',
               'sns.gov': 'ORNL',
               'esss.se': 'ESS',
               'ill.fr': 'ILL',
               'ill.eu': 'ILL',
               'psi.ch': 'PSI',
               'ansto.gov.au': 'ANSTO',
               'MichaelWedel@users.noreply.github.com': 'PSI',
               'stuart.i.campbell@gmail.com': 'ORNL',
               'uwstout.edu': 'ORNL',
               'kitware.com': 'KITWARE',
               'juelich.de': 'JUELICH',
               'ian.bush@tessella.com': 'STFC',
               'dan@dan-nixon.com': 'STFC',
               'peterfpeterson@gmail.com': 'ORNL',
               'stuart@stuartcampbell.me': 'ORNL',
               'harry@exec64.co.uk': 'STFC',
               'martyn.gigg@gmail.com': 'STFC',
               'raquelalvarezbanos@users.noreply.github.com': 'STFC',
               'torben.nielsen@nbi.dk': 'ESS',
               'borreguero@gmail.com': 'ORNL',
               'raquel.alvarez.banos@gmail.com': 'STFC',
               'anton.piccardo-selg@tessella.com': 'STFC',
               'rosswhitfield@users.noreply.github.com': 'ORNL',
               'mareuternh@gmail.com': 'ORNL',
               'quantumsteve@gmail.com': 'ORNL',
               'ricleal@gmail.com': 'ORNL',
               'jawrainey@gmail.com': 'STFC',
               'xingxingyao@gmail.com': 'ORNL',
               'owen@laptop-ubuntu': 'STFC',
               'picatess@users.noreply.github.com': 'STFC',
               'Janik@Janik': 'ORNL',
               'debdepba@dasganma.tk': 'OTHERS',
               'matd10@yahoo.com': 'OTHERS',
               'diegomon93@gmail.com': 'OTHERS',
               'mgt110@ic.ac.uk': 'OTHERS'
               }

    days_in_month = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31]

    csvcommits = open('facility-commits.csv', 'w')
    csvchanged = open('facility-changed-files.csv', 'w')
    csvadded = open('facility-added-lines.csv', 'w')
    csvremoved = open('facility-removed-lines.csv', 'w')

    field_names = ['date']
    for org in organisations:
        field_names.append(org)

    commits_writer = csv.DictWriter(csvcommits, fieldnames=field_names)
    commits_writer.writeheader()

    changed_writer = csv.DictWriter(csvchanged, fieldnames=field_names)
    changed_writer.writeheader()

    added_writer = csv.DictWriter(csvadded, fieldnames=field_names)
    added_writer.writeheader()

    removed_writer = csv.DictWriter(csvremoved, fieldnames=field_names)
    removed_writer.writeheader()

    current_year = int(datetime.datetime.now().strftime("%Y"))
    current_month = int(datetime.datetime.now().strftime("%m"))

    year_start = 2007
    # year_end = year_start
    year_end = current_year

    for year in range(year_start, year_end + 1):
        print("------{0}------".format(str(year)))
        for month in range(1, 13):
            email_changes = ''
            email_commits = ''
            # Don't go past the current month
            if current_year == year:
                if month > current_month:
                    continue

            print("Getting stats for {0}-{1:02d}".format(str(year), month))
            since = "--since='{0}-{1}-1'".format(str(year), str(month))
            until = "--before='{0}-{1}-{2}'".format(str(year), str(month), str(days_in_month[month-1]))

            #
            arg_changes = ['git', 'log', '--pretty=format:"%aE"', '--shortstat', since, until]
            sub = subprocess.Popen(arg_changes, stdout=subprocess.PIPE, close_fds=True, cwd=repolocation)

            date_key = str(year)+'-{0:02d}'.format(month)

            facility_commits[date_key] = {}
            facility_changed[date_key] = {}
            facility_added[date_key] = {}
            facility_removed[date_key] = {}

            # initialize facility counters
            for org in organisations:
                facility_commits[date_key][org] = 0
                facility_changed[date_key][org] = 0
                facility_added[date_key][org] = 0
                facility_removed[date_key][org] = 0

            for line in sub.stdout:
                changed = 0
                added = 0
                removed = 0

                # Is the line blank (or None)
                if line is None:
                    # print("BLANK:'{0}'".format(str(line)))
                    continue
                if len(line.strip().replace('\n', '')) == 0:
                    # print("BLANK:'{0}'".format(str(line)))
                    continue

                # print(line)

                # Is the line an email address ?
                if "@" in line:
                    email_changes = line.strip()
                    # print("EMAIL:'{0}".format(str(line)))
                    continue

                for item in line.split(','):
                    if 'files changed' in item:
                        changed = item.strip().split(' ')[0]
                        # print("FILES CHANGED:{0}".format(changed))
                    if 'insertions(+)' in item:
                        added = item.strip().split(' ')[0]
                        # print ("INSERTIONS:{0}".format(added))
                    if 'deletions(-)' in item:
                        removed = item.strip().split(' ')[0]
                        # print ("DELETIONS:{0}".format(removed))

                found = False
                # Assign each to a facility
                for domain in domains.keys():
                    if domain in email_changes:
                        # ORNL didn't join until 2009
                        if domains[domain] == 'ORNL' and int(year) < 2009:
                            domain = 'stfc.ac.uk'
                        facility_changed[date_key][domains[domain]] += int(changed)
                        facility_added[date_key][domains[domain]] += int(added)
                        facility_removed[date_key][domains[domain]] += int(removed)
                        found = True
                        # print("FILES CHANGED:{0} ==> {1}".format(changed,domains[domain]))
                        # print("FILES CHANGED:{0} ==> {1}".format(added,domains[domain]))
                        # print("FILES CHANGED:{0} ==> {1}".format(removed,domains[domain]))

                # Print out the email address if it didn't match anything
                if not found:
                    print("Email ({0}) couldn't be matched to a facility!".format(str(email_changes)))

            args_commits = ['git', 'shortlog', '-sne', since, until]
            # print(args_commits)
            sub2 = subprocess.Popen(args_commits, stdout=subprocess.PIPE, close_fds=True, cwd=repolocation)
            commits = 0
            for line in sub2.stdout:
                # print(line)
                found = False
                commits, person = [x.strip() for x in line.split("\t")]
                commits = int(commits)
                # Split up person into name and email
                # get rid of the trailing '>' on the email
                person = person.replace(">", "")
                # split on the leading '<' on the email.
                name, email_commits = [x.strip() for x in person.split("<")]
                # data[date_key].append([commits, name, email_commits])

                found = False
                # Assign each to a facility
                for domain in domains.keys():
                    if domain in email_commits:
                        # ORNL didn't join until 2009
                        if domains[domain] == 'ORNL' and int(year) < 2009:
                            domain = 'stfc.ac.uk'
                        facility_commits[date_key][domains[domain]] += commits
                        found = True

                if not found:
                    print("Email for commits ({0}) couldn't be matched to a facility!".format(str(email_commits)))

            commits_datarow = {'date': date_key+"-01"}
            changed_datarow = {'date': date_key+"-01"}
            added_datarow = {'date': date_key+"-01"}
            removed_datarow = {'date': date_key+"-01"}

            # Now actually store the values
            for org in organisations:
                commits_datarow[org] = facility_commits[date_key][org]
                changed_datarow[org] = facility_changed[date_key][org]
                added_datarow[org] = facility_added[date_key][org]
                if lines_removed_as_negative:
                    removed_datarow[org] = -1 * (facility_removed[date_key][org])
                else:
                    removed_datarow[org] = facility_removed[date_key][org]

                print("In {0}, {1} has {2} commits".format(date_key, org, facility_commits[date_key][org]))

            commits_writer.writerow(commits_datarow)
            changed_writer.writerow(changed_datarow)
            added_writer.writerow(added_datarow)
            removed_writer.writerow(removed_datarow)

    csvcommits.close()
    csvchanged.close()
    csvadded.close()
    csvremoved.close()

    print("All done!\n")