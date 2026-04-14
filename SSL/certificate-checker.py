#!/usr/bin/env python3
"""
Script to check SSL certificates for one or more host:port targets and
send a single consolidated email notification if any are expired,
expiring soon, or unreachable.

Designed to be run as a cron job.
"""

import socket
import ssl
import datetime
import smtplib
from email.mime.text import MIMEText
import argparse
import sys
import logging

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler('/home/tridge/cron/cert_checker.log'),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger('cert_checker')


def check_certificate(hostname, port):
    """Return the expiry datetime of the server's TLS certificate."""
    context = ssl.create_default_context()
    conn = context.wrap_socket(
        socket.socket(socket.AF_INET),
        server_hostname=hostname,
    )
    conn.settimeout(3.0)
    try:
        conn.connect((hostname, port))
        cert = conn.getpeercert()
    finally:
        try:
            conn.close()
        except Exception:
            pass
    return datetime.datetime.strptime(cert['notAfter'], '%b %d %H:%M:%S %Y %Z')


def parse_target(t):
    host, _, port = t.rpartition(':')
    if not host or not port:
        raise ValueError(f"target must be host:port, got {t!r}")
    return host, int(port)


def format_hostport(host, port):
    return f"{host}:{port}"


def send_notification_email(recipient, expired, warning, errors, smtp_server="localhost"):
    """Send a single email summarising all problem hosts."""
    counts = []
    if expired:
        counts.append(f"{len(expired)} expired")
    if warning:
        counts.append(f"{len(warning)} expiring")
    if errors:
        counts.append(f"{len(errors)} errors")
    subject = "SSL certificate alert: " + ", ".join(counts)

    lines = []
    if expired:
        lines.append("Expired:")
        for hostport, expiry_date, days_ago in expired:
            lines.append(f"  {hostport} — expired on {expiry_date.strftime('%Y-%m-%d')} ({days_ago} days ago)")
        lines.append("")
    if warning:
        lines.append("Expiring soon:")
        for hostport, expiry_date, days_left in warning:
            lines.append(f"  {hostport} — expires on {expiry_date.strftime('%Y-%m-%d')} (in {days_left} days)")
        lines.append("")
    if errors:
        lines.append("Errors:")
        for hostport, err in errors:
            lines.append(f"  {hostport} — {err}")
        lines.append("")
    lines.append("This is an automated message from the certificate monitoring script.")
    body = "\n".join(lines)

    msg = MIMEText(body)
    msg['Subject'] = subject
    msg['From'] = f"Certificate Monitor <noreply@{socket.gethostname()}>"
    msg['To'] = recipient

    try:
        smtp = smtplib.SMTP(smtp_server)
        smtp.send_message(msg)
        smtp.quit()
        logger.info(f"Notification email sent to {recipient}")
    except Exception as e:
        logger.error(f"Failed to send notification email: {e}")


def main():
    parser = argparse.ArgumentParser(description='Check SSL certificate expiry for one or more servers')
    parser.add_argument('targets', nargs='+', metavar='HOST:PORT',
                        help='One or more targets to check, e.g. mail.example.com:993')
    parser.add_argument('--email', default=None,
                        help='Email to notify if any certificate is expired or expiring')
    parser.add_argument('--smtp-server', default='localhost',
                        help='SMTP server to use for sending mail (default: localhost)')
    parser.add_argument('--days-warning', type=int, default=10,
                        help='Warn if certificate expires within this many days (default: 10)')

    args = parser.parse_args()

    expired = []
    warning = []
    errors = []

    for t in args.targets:
        try:
            host, port = parse_target(t)
        except ValueError as e:
            logger.error(str(e))
            errors.append((t, str(e)))
            continue

        hostport = format_hostport(host, port)
        try:
            expiry_date = check_certificate(host, port)
        except Exception as e:
            logger.error(f"Error checking {hostport}: {e}")
            errors.append((hostport, str(e)))
            continue

        now = datetime.datetime.now()
        time_remaining = expiry_date - now
        days = time_remaining.days
        logger.info(f"Certificate for {hostport} expires on {expiry_date.strftime('%Y-%m-%d')} in {days} days")

        if days < 0:
            logger.warning(f"{hostport}: certificate EXPIRED on {expiry_date.strftime('%Y-%m-%d')}")
            expired.append((hostport, expiry_date, -days))
        elif days <= args.days_warning:
            logger.warning(f"{hostport}: certificate expires in {days} days")
            warning.append((hostport, expiry_date, days))

    if (expired or warning or errors) and args.email:
        send_notification_email(args.email, expired, warning, errors, args.smtp_server)

    sys.exit(1 if (expired or warning or errors) else 0)


if __name__ == "__main__":
    main()
